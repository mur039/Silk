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
#include <dev.h>
#include <ps2.h>
#include <ps2_mouse.h>
#include <kb.h>
#include <timer.h>
#include <pci.h>

#include <v86.h>
#include <glyph.h>
#include <fb.h>
#include <filesystems/tar.h>
#include <filesystems/vfs.h>
#include <syscalls.h>
#include <elf.h>
#include <process.h>
#include <circular_buffer.h>


extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t kernel_phy_end;
extern uint32_t kernel_phy_start;


circular_buffer_t * ksock_buf;

__attribute__((fastcall)) void jump_usermode(uint8_t *function, uint8_t *stack);


static inline reboot(){
    ps2_send_command(0xD1); //write next byte to output port of the controller
    ps2_send_data(0xFE);
}


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;

typedef struct  {
        //    unsigned int   st_dev;      /* ID of device containing file */
        //    unsigned int   st_ino;      /* Inode number */
           unsigned int  st_mode;     /* File type and mode */
        //    unsigned int st_nlink;    /* Number of hard links */
           unsigned int   st_uid;      /* User ID of owner */
           unsigned int   st_gid;      /* Group ID of owner */

} stat_t;

volatile int  is_received_char = 0;
char ch;

volatile int  is_kbd_pressed = 0;
char kb_ch;



#define MAX_OPEN_FILES  8
int opened_file_index = 0;
// file_t opened_files[MAX_OPEN_FILES] = {{.f_inode = 0}};


/*O_RDONLY, O_WRONLY, O_RDWR*/
void syscall_open(struct regs *r){
    //modify for it to be per process
  for(int i = 0; i < MAX_OPEN_FILES; i++){

    r->eax = -1;
    if(current_process->open_descriptors[i].f_inode == NULL){ //find empty entry
        current_process->open_descriptors[i].f_inode = tar_get_file((char *)r->ebx, (int) r->ecx); //open file
        
        if(current_process->open_descriptors[i].f_inode != NULL)
        {
            uart_print(COM1, "Succesfully opened file %s : %x\r\n", r->ebx, i);
            current_process->open_descriptors[i].f_mode = r->ecx;
            current_process->open_descriptors[i].f_pos = 0;  
            r->eax = i;
            return;
        }
        break;
        }
    }

}

//modify for to be per process
void syscall_read(struct regs *r){ //only one byte at a time
    file_t * file = &current_process->open_descriptors[r->ebx];
    if(file->f_inode == NULL){   //check if its opened
        r->eax = -1;
        return;
    }

    if((file->f_mode & (O_RDONLY| O_RDWR)) == 0) { //check if its readable
        r->eax = -1;
        return;
    }

        
    switch (tar_get_filetype(file->f_inode))
    {
    case REGULAR_FILE: //regular read negro
        if(file->f_pos >= o2d(file->f_inode->size)){ //EOF
            r->eax = -1;
        }
        else{
            uint8_t  * result;
            result = (uint8_t*)(&(file->f_inode[1]));
            r->eax = result[file->f_pos];
            file->f_pos += 1;
        }
        break;
            
    case BLOCK_SPECIAL_FILE:
    case CHARACTER_SPECIAL_FILE:
        switch (tar_get_major_number(file->f_inode))
        {
            case 0: //dev/zero
                r->eax = 0;
                break;
            case 1: //ps mouse
                r->eax = ps2_mouse_read();
                break;
            case 2: // /dev/fb
                //it should be read only though
                // r->eax = ((uint8_t *)framebuffer)[file->f_pos];
                // file->f_pos += 1;
                break;
            case 3: //dev/kbd
                if(is_kbd_pressed == 1){
                    is_kbd_pressed = 0;
                    r->eax = kb_ch;
                }
                else{
                    r->eax = -1;    
                }
                break;

            case 4: //tty character device
                if(is_received_char == 1){
                    is_received_char = 0;
                    r->eax = ch;
                }
                else{
                    r->eax = -1;
                }
                break;

            case 107: //ksock //kernel will answer to that
                current_process->state = TASK_STOPPED;
                circular_buffer_getc(&ksock_buf[1]);
                break;
            default:break;
            }
            break;
    default:
        r->eax = -1;
        break;
            
}

    return;
}

void syscall_write(struct regs * r){
    file_t * file;
    file = &current_process->open_descriptors[r->ebx];
    if(file->f_inode == NULL){   //check if its opened
        r->eax = -1;
        return;
    }

    if((file->f_mode & (O_WRONLY | O_RDWR)) == 0) { //check if its writable
        r->eax = -1;
        return;
    }


    switch (tar_get_major_number(file->f_inode)){
        case 0: // /dev zero non writable
        case 1:// /dev/mouse
            r->eax = -1;
            break;
                
        case 2: //dev/fb
            if(file->f_pos + r->edx >= 800*600*4){
                file->f_pos = 0; //rewind
            }
            framebuffer_raw_write(file->f_pos, (void *)r->ecx, r->edx);
            file->f_pos += r->edx;
            r->eax = r->edx;
            
            
            break;

        
        case 4: //tty character device
            switch (tar_get_minor_number(file->f_inode)){
                case 0: //fb
                    fb_console_write((void *)r->ecx, 1, r->edx);
                    break;
                case 1: //com1
                    uart_write(COM1, (void*)r->ecx, 1, r->edx);
                    r->eax = r->edx;
                    break;
        
            }
            break;
        case 107: //ksock
            fb_console_put("From ksock: ");
            fb_console_write((void *)r->ecx, 1, r->edx);
            fb_console_putchar('\n');
            
            char * p8 = (char *)r->ecx;
            for(int i = 0; i < r->edx; ++i){
                circular_buffer_putc(&ksock_buf[0], p8[i]);
            }
            // circular_buffer_write(&ksock_buf, (void *)r->ecx, 1, r->edx);
            break;

        default:
            break;

    }

    return;
}

void syscall_close(struct regs * r){

    if(current_process->open_descriptors[r->ebx].f_inode == NULL){ //trying to closed files that is not opened
        r->eax = -1;
        return;
    }
    else{
        current_process->open_descriptors[r->ebx].f_flags = 0;
        current_process->open_descriptors[r->ebx].f_inode = NULL;
        current_process->open_descriptors[r->ebx].f_mode = 0;
        current_process->open_descriptors[r->ebx].f_pos = 0;
        r->eax = 0;
    }
    return;
}

void syscall_fstat(struct regs * r){
    //ebx = fd, ecx = &stat
    file_t * file;
    file = &current_process->open_descriptors[r->ebx];
    if(file->f_inode == NULL){   //check if its opened
        r->eax = -1;
        return;
    }

    stat_t * stat = (void *)r->ecx;
    stat->st_mode = tar_get_filetype(file->f_inode) << 16 | file->f_flags;
    stat->st_uid = o2d(file->f_inode->uid);
    stat->st_gid = o2d(file->f_inode->gid);
    return;
}


void syscall_lseek(struct regs * r){
    file_t * file;
    file = &current_process->open_descriptors[r->ebx];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
        return;
    }

    if (!(
            tar_get_filetype(file->f_inode) == BLOCK_SPECIAL_FILE ||
            tar_get_filetype(file->f_inode) == REGULAR_FILE))
    { // not a block device nor a normal file so non seekable
        r->eax = -1;
        return;
    }

    long new_pos = 0;
    if (r->edx == SEEK_SET)
    {
        new_pos = r->ecx;
    }
    else if (r->edx == SEEK_CUR)
    {
        new_pos = r->ecx + file->f_pos;
    }
    else if (r->edx == SEEK_END)
    {
        new_pos = o2d(file->f_inode->size) - r->ecx;
    }

    file->f_pos = new_pos;
    r->eax = new_pos;
    // uart_print(COM1, "lseek fd:%x offset:%x whence:%x\r\n", r->ebx, r->ecx, r->edx);

    return;
}

void syscall_exit(struct regs *r){ 
    current_process->state = TASK_ZOMBIE;
    fb_console_printf("Program exited with code %u...\n", r->ebx & 0xff);
    halt();
    schedule(r);
}



void uart_handler(struct regs *r){
    (void)r;
    char c = serial_read(0x3f8);
    is_received_char = 1;
    ch = c;
    // fb_console_printf("%c:%x\n",ch, ch);
    return;
}



void keyboard_handler(struct regs *r)
{
  if(r->err_code == 0){;}
    unsigned char scancode;
    scancode = inb(0x60);
    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    /*
    ctrl 0x1d
    alt 0x38
    left shift 0x2a
    right shift 0x36

    */
    if (scancode & 0x80) //released
    {
        if((scancode &  0x7f) == 0x1d){ //if ctrl is released
            ctrl_flag = 0;
        }else if((scancode &  0x7f) == 0x2a || (scancode &  0x7f) == 0x36 ){ //shift modifier
            shift_flag = 0;
        }
        /* You can use this one to see if the user released the
        *  shift, alt, or control keys... */
    }
    else //pressed
    {
        if((scancode &  0x7f) == 0x1d){ //ctrl modifier
            ctrl_flag = 1;
        }else if( (scancode &  0x7f) == 0x2a || (scancode &  0x7f) == 0x36 ){ //shift modifier
            shift_flag = 1;
        }
        else{

            kb_ch = kbdus[scancode] & (ctrl_flag ? 0x1f : 0x7f);
            if(shift_flag){
                if(kb_ch >= 'a' && kb_ch <= 'z'){  //to upper case
                    kb_ch -= 32;
                    }
                else if(kb_ch >= 'A' && kb_ch <= 'Z'){ //to lower case
                    kb_ch += 32;
                    } //to lower case
            }
            // fb_console_putchar(kb_ch);
            is_kbd_pressed = 1;

        }
        // fb_console_printf( "Key : %c scancode:%u\r\n", kbdus[scancode], scancode);
        return;
        // printf("%u\n", scancode);
    }
}





void syscall_execve(struct regs * r){ //more like spawn 
    r->eax = -1;
    if(tar_get_file(r->ebx, O_RDONLY) == NULL) return;
    

    //leap of faith
    create_process((char *)r->ebx, (char **)r->ecx); //pathname and arguments
    terminate_process(current_process);
    
    r->eax = 1;
    return;
    }


uint32_t kernel_stack[2048];
uint32_t kernel_syscallstack[128];




void kmain(multiboot_info_t* mbd){ //high kernel

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

        if(mmt->type == 1 && mmt->addr_low == 0x100000){ //if available push to anc create bitmap
            pmm_init(mmt->addr_low, mmt->len_low);
            uart_print(COM1, "Initialized pmm_init\r\n");
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

    uart_print(COM1, "Installing Global Descriptor Tables\r\n");
    gdt_install();
    
    set_kernel_stack((uint32_t)&kernel_syscallstack[127]);

    uart_print(COM1, "Installing Interrupt Descriptor Tables\r\n");
    idt_install();

    uart_print(COM1, "Installing ISRs\r\n");
    isrs_install();

    uart_print(COM1, "Installing IRQs\r\n");
    irq_install();
    irq_install_handler(PS2_KEYBOARD_IRQ, keyboard_handler);
    irq_install_handler(4, uart_handler);
    irq_install_handler(PS2_MOUSE_IRQ, ps2_mouse_handler);

    //fiz paging problems
    virt_address_t addr;
    addr.address = (uint32_t)kernel_heap;
    addr.table += 1;
    for( ; addr.table < 1023; addr.table++){
    //   uart_print(COM1, "incremented table %u:%x , physical :%x\r\n",addr.table,  addr.address, get_physaddr((void *)addr.address));
      unmap_virtaddr((void *)addr.address);
    } 

    
    addr.address = (uint32_t)get_physaddr(kernel_heap);
    for(int i = 0; i < 1 + (addr.address - 0x100000)/4096 ; ++i){
        pmm_mark_allocated((void *)0x100000 + 0x1000*i);
    }



    for(int i = 0; i < (modules[0].mod_end - modules[0].mod_start)/4096 ; ++i){
        map_virtaddr(
            (void *)modules[0].mod_start + i*0x1000,
            (void *)modules[0].mod_start + i*0x1000,
            PAGE_PRESENT | PAGE_READ_WRITE
        );
        
        
        pmm_mark_allocated((void *)modules[0].mod_start + i*0x1000);

    }

    flush_tlb();


    
    kmalloc_init(3);
    alloc_print_list();
    
    vfs_init();
    
    fb_console_printf("Initializing Device Manager\n");
    dev_init();
    
    

    //turn it into to driver of sort?
    tar_add_source((void *)kernel_heap);
    for(int i = 0; i < (modules[0].mod_end - modules[0].mod_start)/4096 ; ++i){
        map_virtaddr(kernel_heap, modules->mod_start + i*0x1000, PAGE_PRESENT | PAGE_READ_WRITE);
        kernel_heap += 0x1000;
    }

    //again a bit ugly innit
    file_t font;    
    font.f_inode = tar_get_file("/share/screenfonts/consolefont_14.psf", O_RDONLY);
    if(font.f_inode == NULL){
        uart_print(COM1, "Failed to open font\r\n");
    }
    font.f_mode = O_RDONLY; 
    parse_psf(&(font.f_inode[1]));
   
    init_framebuffer((void*)mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height);
    init_fb_console(-1, -1); //maximum size //requires fonts tho
    fb_set_console_color((pixel_t){.green = 0xff}, (pixel_t){.blue = 0x00 });


    
    initialize_syscalls();
    install_syscall_handler(SYSCALL_OPEN, syscall_open);
    install_syscall_handler(SYSCALL_EXIT, syscall_exit);
    install_syscall_handler(SYSCALL_READ, syscall_read);
    install_syscall_handler(SYSCALL_WRITE, syscall_write);
    install_syscall_handler(SYSCALL_CLOSE, syscall_close);
    install_syscall_handler(SYSCALL_LSEEK, syscall_lseek);
    install_syscall_handler(SYSCALL_EXECVE, syscall_execve);
    install_syscall_handler(SYSCALL_FSTAT, syscall_fstat);
    


    /*let's create a server in kernel space where application can communicate with kernel
    through a byte or text encoded protocol.
    why? well i want to test some things and my vfs and devices as files are not very flexiable
    it will be on /dev/ksock kernel will listen for '\n' or some bytes? i don't know the protocol yets
    */

    //let's try circular buffer;
    ksock_buf = kmalloc(2*sizeof(circular_buffer_t));
    ksock_buf[0] = circular_buffer_create(64);
    ksock_buf[1] = circular_buffer_create(64);

//    device_t ksock;
//    ksock.name = "ksock";
//    ksock.dev_type = DEVICE_CHAR;
//    ksock.unique_id = dev_register(&ksock);
//    list_devices();
    
    timer_install();
    timer_phase(10);
    
    enumerate_pci_devices();
    enable_interrupts();
    
    ps2_mouse_initialize();

    
    // device_t * mouse = kcalloc(1, sizeof(device_t));
    // mouse->dev_type = DEVICE_CHAR;
    // mouse->name = "mouse";
    // mouse->read = ps2_mouse_fs_read;
    // mouse->write = NULL;
    
    // mouse->unique_id = dev_register(mouse);

    process_init();


    v86_create_task("/bin/v86.bin");
    // while(1);


    const char * init_args[] = {
        "/bin/init",
        "The quick brown fox jumps over the lazy dog",
        NULL
    };

    // create_process("/bin/init", init_args);
   
    
    register_timer_callback(schedule);
    extern int ie_func[];
    map_virtaddr(ie_func, ie_func, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    jump_usermode(ie_func, ie_func);
    
    
    //we shouldn't get there
    for(;;)
    {
    
    }

}
