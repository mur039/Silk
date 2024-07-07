#include <multiboot.h>
#include <stdint.h>
#include <sys.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
#include <acpi.h>   
#include <pmm.h>
#include <str.h>
#include <pit.h>
#include <gdt.h>
#include <kb.h>
#include <timer.h>

#include <glyph.h>
#include <fb.h>
#include <filesystems/tar.h>

extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t kernel_phy_end;

__attribute__((fastcall)) void jump_usermode(uint8_t *function, uint8_t *stack);


/* convention
retvalue = 	eax
syscall number = eax	    
arg0 = ebx	
arg1 = ecx	
arg2 = edx	
arg3= esi	
arg4 = edi	
arg5 = ebp
*/

uint32_t * framebuffer;

typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;

enum syscall_numbers{
    SYSCALL_READ = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_OPEN = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_LSEEK = 4,
    SYSCALL_EXIT = 60
};


volatile int  is_received_char = 0;
char ch;

volatile int  is_kbd_pressed = 0;
char kb_ch;


typedef long off_t;
typedef struct  {
	unsigned short f_mode;
	unsigned short f_flags;
	tar_header_t *f_inode;
	off_t f_pos;
} file_t;

#define MAX_OPEN_FILES  8
int opened_file_index = 0;
file_t opened_files[MAX_OPEN_FILES] = {{.f_inode = 0}};

void syscall_handler(struct regs *r){
    int result = -1;
    file_t * file;

    switch((uint8_t)r->eax){

        case SYSCALL_READ:

            file = &opened_files[r->ebx];
            if(file->f_inode == NULL){   //check if its opened
                r->eax = -1;
                break;
            }


            if((file->f_mode & (O_RDONLY| O_RDWR)) == 0) { //check if its readable
                r->eax = -1;
                break;
            }


            switch (file->f_inode->devmajor[6] - '0')
            {
                case 1: //dev/zerp 
                    r->eax = 0;
                    break;

                case 2: // /dev/fb
                    r->eax = ((uint8_t *)framebuffer)[file->f_pos];
                    file->f_pos += 1;
                    break;

                case 3: //dev/kbd
                    if(is_kbd_pressed == 1){
                        result = kb_ch;
                        is_kbd_pressed = 0;
                    }
                    r->eax = result;
                    break;

                case 4: //tty character device
                    if(is_received_char == 1){
                        result = ch;
                        is_received_char = 0;
                    }
                    r->eax = result;
                    break;

                default:
                    break;
            }

            break;

        case SYSCALL_WRITE:
        
            file = &opened_files[r->ebx];
            if(file->f_inode == NULL){   //check if its opened
                r->eax = -1;
                break;
            }

            if((file->f_mode & (O_WRONLY | O_RDWR)) == 0) { //check if its writable
                r->eax = -1;
                break;
            }

            switch (file->f_inode->devmajor[6] - '0'){
                case 1: //dev zero non writable
                    r->eax = -1;
                    break;
                
                case 2: //dev/fb
                    memcpy( &((uint8_t*)framebuffer)[file->f_pos], (void *)r->ecx, r->edx);
                    file->f_pos += r->edx;
                    r->eax = r->edx;
                    
                    break;

                case 4: //tty character device
                    switch (file->f_inode->devminor[6] - '0'){
                        case 0: //fb
                            fb_console_write((void *)r->ecx, 1, r->edx);
                            break;
                        case 1: //com1
                            uart_write(COM1, (void*)r->ecx, 1, r->edx);
                            r->eax = r->edx;
                            break;
                    }
                    break;

                default:
                    break;
            }

            break;

        /*O_RDONLY, O_WRONLY, O_RDWR*/
        case SYSCALL_OPEN:

            for(int i = 0; i < MAX_OPEN_FILES; i++){

                if(opened_files[i].f_inode == NULL){ //find empty entry
                    r->eax = -1;
                    opened_files[i].f_inode = tar_get_file((char *)r->ebx, (int) r->ecx);

                    if(opened_files[i].f_inode != NULL)
                    {
                        uart_print(COM1, "Succesfully opened file %s : %x\r\n", r->ebx, i);
                        opened_files[i].f_mode = r->ecx;
                        r->eax = i;
                    }

                    break;
                }

            }
            break;

        case SYSCALL_CLOSE: // close(int fd)
            if(opened_files[r->ebx].f_inode == NULL){ //trying to closed files that is not opened
                r->eax = -1;
                break;
            }
            else{
                opened_files[r->ebx].f_flags = 0;
                opened_files[r->ebx].f_inode = NULL;
                opened_files[r->ebx].f_mode = 0;
                opened_files[r->ebx].f_pos = 0;
                r->eax = 0;

            }
            break;

        case SYSCALL_LSEEK: // lseek(int fd, off_T offset, int whence)

            file = &opened_files[r->ebx];
            if(file->f_inode == NULL){   //check if its opened
                r->eax = -1;
                break;
            }
            if( !(
                tar_get_filetype(file->f_inode) == BLOCK_SPECIAL_FILE
                ||
                tar_get_filetype(file->f_inode) == REGULAR_FILE
                    )
                ){ //not a block device nor a normal file so non seekable 
                r->eax = -1;
                break;
            }

            long new_pos = 0;
            if(r->edx == SEEK_SET){
                new_pos = r->ecx;
                }
            else if(r->edx == SEEK_CUR){
                new_pos = r->ecx + file->f_pos; 
            }
            else if(r->edx == SEEK_END){}

            file->f_pos = new_pos;
            r->eax = new_pos;
            //uart_print(COM1, "lseek fd:%x offset:%x whence:%x\r\n", r->ebx, r->ecx, r->edx);
            break;

        case SYSCALL_EXIT:
            uart_print(COM1, "Program exited with code %x...\r\n", r->ebx);
            halt();
            
        default:
            uart_print(COM1, "Unkown system call %x\r\n", r->eax);
            halt();
            break;
    }
    return;
}


void uart_handler(struct regs *r){
    (void)r;
    char c = serial_read(0x3f8);
    is_received_char = 1;
    ch = c;
    return;
}



void keyboard_handler(struct regs *r)
{
  if(r->err_code == 0){;}
    unsigned char scancode;
    scancode = inb(0x60);
    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    
    if (scancode & 0x80)
    {
        /* You can use this one to see if the user released the
        *  shift, alt, or control keys... */
    }
    else
    {
        kb_ch = kbdus[scancode];
        // fb_console_putchar(kb_ch);
        is_kbd_pressed = 1;
        return;
        // uart_print(COM1, "Key : %c scancode:%x\r\n", kbdus[scancode], scancode);
        // printf("%u\n", scancode);
    }
}



uint32_t kernel_stack[2048];
uint32_t kernel_syscallstack[128];


 __attribute__((noreturn)) void kmain(multiboot_info_t* mbd){ //high kernel

    init_uart_port(COM1); //COM1
    

    uart_print(COM1, "Initializing Pyhsical Memory Manager\r\n");
    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(struct multiboot_mmap_entry)){

        // mmt->type; //look for 1;
        multiboot_memory_map_t * mmt = (multiboot_memory_map_t *)(mbd->mmap_addr +i);
        
        // sprintf
        uart_print(COM1, "%x...%x : Size:%x -> Type : %s\r\n", 
        mmt->addr_low,
        mmt->addr_low + mmt->len_low,
        mmt->len_low,
        mmt->type == 1?
        "Available" :
        "Reserved"
        );

        if(mmt->type == 1){ //if available push to anc create bitmap

        }
    }



    uart_print(COM1, "Modules found : %x, At : %x\r\n", mbd->mods_count, mbd->mods_addr);
    multiboot_module_t * modules  = (multiboot_module_t *)mbd->mods_addr;
    for(unsigned int i = 0; i < mbd->mods_count; i++){
        uart_print(COM1, 
        "Module %x:\r\n"
        "\tcmdline: %s\r\n"
        "\tstart address: 0x%x\r\n"
        "\tend address  : 0x%x\r\n"
        "\tmodule size  : %x\r\n" ,
        i,
        (char*)modules[i].cmdline,
        modules[i].mod_start,
        modules[i].mod_end,
        modules[i].mod_end - modules[i].mod_start
        );

    }

    tar_add_source((void *)modules[1].mod_start);
	// tar_parse();

    file_t font;    
    font.f_inode = tar_get_file("/share/screenfonts/consolefont_14.psf", O_RDONLY);
    if(font.f_inode == NULL){
        uart_print(COM1, "Failed to open font\r\n");
    }
    font.f_mode = O_RDONLY; 
    parse_psf(&(font.f_inode[1]));
   

    init_framebuffer((void*)mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height);
    init_fb_console(-1, -1); //maximum size





    uart_print(COM1, "Installing Global Descriptor Tables\r\n");
    gdt_install();
    
    set_kernel_stack((uint32_t)&kernel_syscallstack[127]);

    uart_print(COM1, "Installing Interrupt Descriptor Tables\r\n");
    idt_install();

    uart_print(COM1, "Installing ISRs\r\n");
    isrs_install();

    uart_print(COM1, "Installing IRQs\r\n");

    irq_install();
    irq_install_handler(1, keyboard_handler);
    irq_install_handler(4, uart_handler);
    
    
    timer_install();
    timer_phase(1);
    //ACPI initialization
    // struct RSDP_t rdsp;
    // rdsp = find_rsdt();//this fella is well above the first 1M so we have to do something about it

    enable_interrupts();
    
    
    //loading userprogram.bin to memory and possibly execute
    uint8_t * bumpy  = (void*)&kernel_phy_end, * bumpy_stack;


    bumpy = (void*)0x11e000;
    bumpy = align2_4kB(bumpy);
    bumpy_stack = bumpy + 4095;


    map_virtaddr(bumpy, modules->mod_start, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    
    bumpy_stack = bumpy + 4095;
    
    flush_tlb();

    jump_usermode(bumpy, bumpy_stack);

    


    for(;;)
    {

    }

}
