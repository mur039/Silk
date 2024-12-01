#include <multiboot.h>
#include <stdint-gcc.h>
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
#include <cmos.h>
#include <ps2_mouse.h>
#include <kb.h>
#include <timer.h>
#include <pci.h>
#include <virtio.h>

#include <v86.h>
#include <glyph.h>
#include <fb.h>
#include <filesystems/tar.h>
#include <filesystems/vfs.h>
#include <syscalls.h>
#include <elf.h>
#include <process.h>
#include <circular_buffer.h>
#include <pipe.h>

extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t kernel_phy_end;
extern uint32_t kernel_phy_start;


circular_buffer_t * ksock_buf;

__attribute__((fastcall)) void jump_usermode(void *function, void *stack);


static inline void reboot_by_ps2(){ //there's no way i can return
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

typedef struct  {
    uint32_t d_ino;
    uint32_t d_off;      
    unsigned short d_reclen;    /* Length of this record */
    int  d_type;      /* Type of file; not supported*/
    char d_name[256]; /* Null-terminated filename */
} dirent_t;



volatile int  is_received_char = 0;
char ch;

volatile int  is_kbd_pressed = 0;
char kb_ch;




int opened_file_index = 0;
// file_t opened_files[MAX_OPEN_FILES] = {{.f_inode = 0}};

int32_t open_for_process(pcb_t * process, const char * path, int mode ){
     
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        if(process->open_descriptors[i].f_inode == NULL){ //find empty entry
            process->open_descriptors[i].f_inode = tar_get_file(path, mode); //open file
        
            if(process->open_descriptors[i].f_inode != NULL) {
                uart_print(COM1, "Succesfully opened file %s : %x\r\n", path, i);
                process->open_descriptors[i].f_mode = mode;
                process->open_descriptors[i].f_pos = tar_get_filetype(process->open_descriptors[i].f_inode) == DIRECTORY ? 1 : 0;  
                return i;
                
            }
            else{
                return -1;
            }
            
        }
    }
    return -1;

}

/*O_RDONLY, O_WRONLY, O_RDWR*/
void syscall_open(struct regs *r){
    r->eax = open_for_process(current_process, (char *)r->ebx, (int)r->ecx ) ;
}

//modify for to be per process
void syscall_read(struct regs *r){ //only one byte at a time

    int fd = (int)r->ebx;
    u8* buf = (u8*)r->ecx;
    size_t count = (size_t)r->edx;

    file_t * file = &current_process->open_descriptors[fd];
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
            r->eax = 0;
        }
        else{
            uint8_t  * result;
            result = (uint8_t*)(&(file->f_inode[1]));
            buf[0] = result[file->f_pos];
            file->f_pos += 1;
            r->eax = 1;
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
                    *buf = kb_ch;
                }
                else{
                    //actually block but
                    r->eax = 0; //read size of 0    
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
                // current_process->state = TASK_STOPPED;
                r->eax = circular_buffer_getc(ksock_buf);
                break;
            default:
                break;
            }
            break;
    
    case DIRECTORY:
        ;
        //assume count equals to sizeof(struct dirent)
        // fb_console_printf("reading a directory in syscall fd: %u\n", fd);
        dirent_t * dir = (dirent_t*)buf;
        tar_header_t* h;

        if( tar_read_dir(file, &h) == -1 ){
            r->eax = 0;
            break;
        }
        
        file->f_pos += 1;
        
        r->eax = count;
        if(dir){
            memcpy(dir->d_name, &h->filename[1], 99);
            dir->d_type = tar_get_filetype(h);
        }
        
        break;

    case FIFO_SPECIAL_FILE: //for both named and unnamed pipes
        ;
        pipe_t * ps = (pipe_t *)&current_process->open_descriptors[fd].f_inode[1];
        ps = &ps[-1];

        int ret = circular_buffer_getc(&ps->cb);
        if(ret == -1){
            r->eax = 0;
            return;
        }

        *buf = ret & 0xff;
        r->eax = 1;

        break;
        
    default:
        r->eax = 0;
        break;
            
}

    return;
}

void syscall_write(struct regs * r){

    int fd = (int)r->ebx;
    u8* buf = (u8*)r->ecx;
    size_t count = (size_t)r->edx;

    file_t * file;
    file = &current_process->open_descriptors[fd];
    if(file->f_inode == NULL){   //check if its opened
        r->eax = -1;
        return;
    }

    if((file->f_mode & (O_WRONLY | O_RDWR)) == 0) { //check if its writable
        r->eax = -1;
        return;
    }


     switch (tar_get_filetype(file->f_inode))
    {
        
        case BLOCK_SPECIAL_FILE:
        case CHARACTER_SPECIAL_FILE:
            switch (tar_get_major_number(file->f_inode)){
                case 0: // /dev zero non writable
                case 1:// /dev/mouse
                    
                    r->eax = -1;
                    break;

                case 2: //dev/fb
                    
                    if(file->f_pos + r->edx >= 800*600*4){
                        file->f_pos = 0; //rewind
                    }
                    framebuffer_raw_write(file->f_pos, (void *)buf, count);
                    file->f_pos += count;
                    r->eax = count;
                    break;


                case 4: //tty character device
                    
                    switch (tar_get_minor_number(file->f_inode)){
                        case 0: //fb
                            fb_console_write((void *)buf, 1, count);
                            break;
                        case 1: //com1
                            uart_write(COM1, (void*)buf, 1, count);
                            r->eax = count;
                            break;

                    }
                    break;

                case 107: //ksock
                    fb_console_put("From ksock: ");
                    fb_console_write((void *)buf, 1, count);
                    fb_console_putchar('\n');

                    char * p8 = (char *)buf;
                    for(unsigned int i = 0; i < count; ++i){
                        circular_buffer_putc(ksock_buf, p8[i]);
                    }

                    // circular_buffer_write(&ksock_buf, (void *)r->ecx, 1, r->edx);
                    break;

                default:
                    break;

            }
            break;
        
        case FIFO_SPECIAL_FILE:
            ;
            // fb_console_printf("a write to the pipe darling");
            pipe_t * ps = (pipe_t *)&current_process->open_descriptors[fd].f_inode[1];
            ps = &ps[-1];
            
            if(!buf){
                r->eax = 0;
                return;
            }
            

            if( circular_buffer_putc(&ps->cb, *buf) == -1){
                r->eax = 0;
                return;
            }

            r->eax = 1;

            break;
        


        default:
            r->eax = 0;
            break;
    };


    

    return;
}



int close_for_process(pcb_t * process, int fd){
    
    if(process->open_descriptors[fd].f_inode == NULL){ //trying to closed files that is not opened
        return -1;
    }

    if( tar_get_filetype(process->open_descriptors[fd].f_inode) == FIFO_SPECIAL_FILE){
        
        pipe_t * ps = (void *)&process->open_descriptors[fd].f_inode[1];
        ps = &ps[-1];

        if(tar_get_filetype(process->open_descriptors[fd].f_mode) == O_RDONLY){ //close ref_Count
            ps->read_refcount = (ps->read_refcount - 1) > 0 ? (ps->read_refcount - 1) : 0;
        }
        else{
            ps->write_refcount = (ps->write_refcount - 1) > 0 ? (ps->write_refcount - 1) : 0;
        }

        //no references exist so deallocate the pipe
        if(!ps->write_refcount && !ps->read_refcount){
            circular_buffer_destroy(&ps->cb);
            kfree(process->open_descriptors[fd].f_inode);
        }

    }

    
    process->open_descriptors[fd].f_flags = 0;
    process->open_descriptors[fd].f_inode = NULL;
    process->open_descriptors[fd].f_mode = 0;
    process->open_descriptors[fd].f_pos = 0;
    return 0;
    

}

void syscall_close(struct regs * r){

    r->eax = close_for_process(current_process, (int)r->ebx);
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
    r->eax = 0;
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


    int exitcode = (int)r->ebx;
    r->eax = exitcode;
    current_process->state = TASK_ZOMBIE;

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
        else if( ( scancode & 0x7f) == 0x38 ){ //alt_gr
            alt_gr_flag = 0;
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
        else if( ( scancode & 0x7f) == 0x38 ){ //alt_gr
            alt_gr_flag = 1;
        }
        else{

            kb_ch = kbdus[scancode & 0x7f];//& (ctrl_flag ? 0x1f : 0x7f);

            if( shift_flag && ctrl_flag && alt_gr_flag){ //very unlikely
                if(kb_ch == '<'){
                    fb_console_printf("lucy was high and so was i\n");
                }
            }
            else if( !shift_flag &&  ctrl_flag &&  alt_gr_flag){}
            else if(  shift_flag && !ctrl_flag &&  alt_gr_flag){}

            else if( !shift_flag && !ctrl_flag &&  alt_gr_flag){ //only alt_gr

            }
            else if(  shift_flag &&  ctrl_flag && !alt_gr_flag){}

            else if( !shift_flag &&  ctrl_flag && !alt_gr_flag){ //only ctrl
                kb_ch &= 0x1f;
            }
            else if(  shift_flag && !ctrl_flag && !alt_gr_flag){ //only shift

                switch (kb_ch){
                    case '<':
                        kb_ch = '>';
                        break;
                    case 'a'...'z':
                        kb_ch -= 32;
                        break;

                    case 'A' ... 'Z':
                        kb_ch += 32;
                        break;

                    default: 
                        // fb_console_printf("%u : %u\n", kb_ch, scancode);
                    break;
                }


            }
            else if( !shift_flag && !ctrl_flag && !alt_gr_flag){ }



            // if(shift_flag){
            //     if(scancode == 86){ //< in turkish ke
            //         kb_ch  += 2;
            //     }
            //     else if(kb_ch >= 'a' && kb_ch <= 'z'){  //to upper case
            //         kb_ch -= 32;
            //         }
            //     else if(kb_ch >= 'A' && kb_ch <= 'Z'){ //to lower case
            //         kb_ch += 32;
            //         } //to lower case
                    
            // }
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
    if(tar_get_file((const char *)r->ebx, O_RDONLY) == NULL) return;
    
    //leap of faith
    pcb_t * np = create_process((char *)r->ebx, (char **)r->ecx); //pathname and arguments
    
    //copy file descriptors
    memcpy(np->open_descriptors, current_process->open_descriptors, MAX_OPEN_FILES * sizeof(file_t));
    np->parent = current_process->parent;
    np->childs = current_process->childs;
    
    current_process->state = TASK_ZOMBIE;
    //force schedule?
    r->eax = 1;
    schedule(r);
    
    return;
}


/*
    int dup2(int oldfd, int newfd);
*/
void syscall_dup2(struct regs * r){
    r->eax = -1; //err

    int old_fd = (int)r->ebx;
    int new_fd = (int)r->ecx;
    
    fb_console_printf("SYSCALL_DUP2, old_fd new_fd : %u %u\n", old_fd, new_fd);
    //check whether given old_fd is valid
    if(!current_process->open_descriptors[old_fd].f_inode){ //emty one so err
        return;
    }

    //check if new_fd is open, is so close it
    if(current_process->open_descriptors[new_fd].f_inode){ //open file_desc
        close_for_process(current_process, new_fd);
    }

    current_process->open_descriptors[new_fd] = current_process->open_descriptors[old_fd];
    r->eax = new_fd;
    return;
}



 
void syscall_fork(struct regs * r){ //the show begins
    
    r->eax = -1;
    save_context(r, current_process); //save the latest context
    
    pcb_t* child = create_process( current_process->filename, current_process->argv);
    child->parent = current_process->self;

    
    list_t *clist;
    listnode_t * cnode;    


    list_t *list;
    listnode_t * node;    


    list = current_process->mem_mapping;
    node = list->head;

    clist = child->mem_mapping;
    cnode = clist->head;

    for(int i = 0; i < list->size  ; ++i){

        vmem_map_t * vm = node->val;
        vmem_map_t * cm = kmalloc(sizeof(vmem_map_t));


        //create page for the child, set it to zero
        u8 * ch_page = kpalloc(1);
        memset(ch_page, 0, 4096);
        
        //create the same linked page list for the child
        cm->vmem = vm->vmem;
        cm->phymem = (uint32_t)get_physaddr( ch_page );
        list_insert_end(clist, cm);
        
        //mapping calling process phy page to memory window
        map_virtaddr(memory_window, vm->phymem, PAGE_PRESENT | PAGE_READ_WRITE); 
        memcpy(ch_page, memory_window, 4096); //copying calling process page to child's
        map_virtaddr_d(child->page_dir, cm->vmem, cm->phymem, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);

        //unmap the ch_page
        unmap_virtaddr(ch_page);
        kernel_heap -= 0x1000;

        //increment to the next node
        node = node->next;
    }



    child->argc = current_process->argc;
    child->argv = current_process->argv; //nuh uh

    u32 old_cr3 = child->regs.cr3;
    child->regs = current_process->regs;
    child->regs.cr3 = old_cr3;

    

    memcpy(child->open_descriptors, current_process->open_descriptors, MAX_OPEN_FILES * sizeof(file_t));
    //if there's a unnamed pipe i should increment the references as well
    for(int fd = 0; fd > MAX_OPEN_FILES; ++fd){
        
        if( tar_get_filetype(child->open_descriptors[fd].f_inode) == FIFO_SPECIAL_FILE){

            pipe_t * ps = (void *)&child->open_descriptors[fd].f_inode[1];
            ps = &ps[-1];

            ps->read_refcount++;
            ps->write_refcount++;

        }
    }

    child->state = TASK_RUNNING;


    r->eax = child->pid;
    child->regs.eax = 0;

    schedule(r);
    
    return;
}

void syscall_getpid(struct regs * r){
    r->eax = current_process->pid;
    return;
}


void syscall_pipe(struct regs * r){
    int fd[2] = {-1, -1};

    for(int f = 0; f < 2; f++){
        for(int i = 0; i < MAX_OPEN_FILES; i++){
            if(current_process->open_descriptors[i].f_inode == NULL){ //find empty entry
                current_process->open_descriptors[i].f_inode = 1;
                fd[f] = i;
                break;
            }
    }    
    }

    if( fd[0] == -1 || fd[1] == -1){
        r->eax = -1;
        return;
    }

    int *filds = (int*)r->ebx; //int fd[2]

    file_t pipe = pipe_create(64);

    //read head
    current_process->open_descriptors[ fd[0] ] = pipe;
    current_process->open_descriptors[ fd[0] ].f_mode = O_RDONLY;

    //write head
    current_process->open_descriptors[ fd[1] ] = pipe;
    current_process->open_descriptors[ fd[1] ].f_mode = O_WRONLY;
    filds[0] = fd[0];
    filds[1] = fd[1];
    r->eax = 0;
    return;
}

//pid_t wait4(pid_t pid, int * wstatus, int options, struct rusage * rusage);
void syscall_wait4(struct regs * r){
    pid_t pid = (pid_t)r->ebx;
    int * wstatus = (int *)r->ecx;
    int options = (int)r->edx;
    //rusage is ignored for now


    /*
    for now its gonna be just wait for anything and in terminate process
    we will set status to running again when child get terminated
    */

   current_process->state = TASK_INTERRUPTIBLE;
   schedule(r);

}


//for test purposes
void syscall_yield(struct regs *r ){
    fb_console_printf("this process tried to yield: %u:%s\n", current_process->pid, current_process->filename);
    schedule(r);

}



void syscall_stop(struct regs *r ){
    
    fb_console_printf("syscall_Stop called %u\n", r->ebx);
    register_timer_callback( (void *)(((u32)schedule) * r->ebx ? 1 : 0) );
    
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
    for(unsigned int i = 0; i < 1 + (addr.address - 0x100000)/4096 ; ++i){
        pmm_mark_allocated((void *)0x100000 + 0x1000*i);
    }



    for(unsigned int i = 0; i < (modules[0].mod_end - modules[0].mod_start)/4096 ; ++i){
        map_virtaddr(
            (void *)modules[0].mod_start + i*0x1000,
            (void *)modules[0].mod_start + i*0x1000,
            PAGE_PRESENT | PAGE_READ_WRITE
        );
        
        
        pmm_mark_allocated((void *)modules[0].mod_start + i*0x1000);

    }

    

    //turn it into to driver of sort?
    tar_add_source((void *)kernel_heap);
    for(unsigned int i = 0; i < (modules[0].mod_end - modules[0].mod_start)/4096 ; ++i){
        map_virtaddr(kernel_heap, (void *)(modules->mod_start + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE);
        kernel_heap += 0x1000;
    }


    flush_tlb();


    pmm_print_usage();
    kmalloc_init(4);
    alloc_print_list();
    
    vfs_init();
   
   

    //again a bit ugly innit
    file_t font;    
    font.f_inode = tar_get_file("/share/screenfonts/consolefont_14.psf", O_RDONLY);
    if(font.f_inode == NULL){
        uart_print(COM1, "Failed to open font\r\n");
    }
    font.f_mode = O_RDONLY; 
    parse_psf(&(font.f_inode[1]));
   
    uart_print(COM1, "Framebuffer at %x : %u %u %ux%u\r\n", 
                mbd->framebuffer_addr_lower,
                mbd->framebuffer_bpp,
                mbd->framebuffer_pitch,
                mbd->framebuffer_width,
                mbd->framebuffer_height
                 );
    init_framebuffer((void*)mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height);
    init_fb_console(-1, -1); //maximum size //requires fonts tho
    fb_set_console_color((pixel_t){.green = 0xff}, (pixel_t){.blue = 0x00 });

 
    

    rsdp_t rsdp = find_rsdt();
    if(!rsdp_is_valid(rsdp)){
        fb_console_put("Failed to find rsdp in the memory, halting...");
        uart_print(COM1, "Failed to find rsdp in the memory, halting...");
        halt();
    }


    fb_console_put("RSDP:\n");

    fb_console_put("\tSignature: ");fb_console_putchar('\"');
    for(int i = 0; i < 8; ++i){
        fb_console_putchar(rsdp.Signature[i]);
    }
    fb_console_put("\"\n");


    fb_console_put("\tOEMID: ");fb_console_putchar('\"');
    for(int i = 0; i < 6; ++i){
        fb_console_putchar(rsdp.OEMID[i]);
    }
    fb_console_put("\"\n");


    fb_console_printf("\tRevision: %u\n", rsdp.Revision);
    fb_console_printf("\tRsdt: %x\n", rsdp.RsdtAddress);

    RSDT_t * rsdt = (void *)rsdp.RsdtAddress;
    if(!is_virtaddr_mapped(rsdt) ){
        map_virtaddr(rsdt, rsdt, PAGE_PRESENT | PAGE_READ_WRITE);
    }
    
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;

    for(int i = 0; i < entries; ++i){
        fb_console_printf( "%u -> Entry address: %x\n", i, rsdt->PointerToOtherSDT[i]);
    }

    

    fb_console_printf("Initializing Device Manager\n");
    dev_init();
    
    
    initialize_syscalls();
    install_syscall_handler(SYSCALL_OPEN, syscall_open);
    install_syscall_handler(SYSCALL_EXIT, syscall_exit);
    install_syscall_handler(SYSCALL_READ, syscall_read);
    install_syscall_handler(SYSCALL_WRITE, syscall_write);
    install_syscall_handler(SYSCALL_CLOSE, syscall_close);
    install_syscall_handler(SYSCALL_LSEEK, syscall_lseek);
    install_syscall_handler(SYSCALL_EXECVE, syscall_execve);
    install_syscall_handler(SYSCALL_FSTAT, syscall_fstat);
    install_syscall_handler(SYSCALL_FORK, syscall_fork);
    install_syscall_handler(SYSCALL_DUP2, syscall_dup2);
    install_syscall_handler(SYSCALL_GETPID, syscall_getpid);
    install_syscall_handler(SYSCALL_PIPE, syscall_pipe);
    install_syscall_handler(SYSCALL_WAIT4, syscall_wait4);
    install_syscall_handler(63, syscall_stop);
    install_syscall_handler(64, syscall_yield);


    


    /*let's create a server in kernel space where application can communicate with kernel
    through a byte or text encoded protocol.
    why? well i want to test some things and my vfs and devices as files are not very flexiable
    it will be on /dev/ksock kernel will listen for '\n' or some bytes? i don't know the protocol yets
    */

    // let's try circular buffer;
   ksock_buf = kmalloc(sizeof(circular_buffer_t));
   *ksock_buf = circular_buffer_create(64);
   

   device_t ksock;
   ksock.name = "ksock";
   ksock.dev_type = DEVICE_CHAR;
   ksock.unique_id = dev_register(&ksock);
   list_devices();


    timer_install();
    timer_phase(500);
    register_timer_callback(schedule);
    
    enumerate_pci_devices();
    

    fb_console_printf("Found PCI devices: %u\n", pci_devices.size);
    for(listnode_t * node = pci_devices.head; node != NULL ;node = node->next ){
        pci_device_t *dev = node->val;

        if( dev->header.common_header.vendor_id == 0x1af4){
            virtio_register_device(dev);

        }
        

    }



    ps2_mouse_initialize();
    // device_t * mouse = kcalloc(1, sizeof(device_t));
    // mouse->dev_type = DEVICE_CHAR;
    // mouse->name = "mouse";
    // mouse->read = ps2_mouse_fs_read;
    // mouse->write = NULL;
    // mouse->unique_id = dev_register(mouse);
    
    process_init();


    // v86_create_task("/bin/v86.bin");
    // while(1);


    char * init_args[] = {
        "/bin/init",
        NULL
    };

    pcb_t * init =  create_process("/bin/init", init_args);
    // init->parent = NULL;
    
    
    
    extern int ie_func[];
    map_virtaddr(ie_func, ie_func, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    
    
    // fb_console_printf("from rtc year is %u\n",cmos_read_register(CMOS_STATUS_B_REG) );

    enable_interrupts();
    jump_usermode(ie_func, ie_func);

    
    //we shouldn't get there
    for(;;)
    {
    
    }

}
