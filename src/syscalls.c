#include <syscalls.h>
#include <isr.h>
#include <pmm.h>
#include <vmm.h>
#include <process.h>
#include <ps2_mouse.h>

#define MAX_SYSCALL_NUMBER 128
void (*syscall_handlers[ MAX_SYSCALL_NUMBER ])(struct regs *r);


void unkown_syscall(struct regs *r){
    uart_print(COM1, "Unkown syscall %x\r\n", r->eax);
    r->eax = -1;
}

extern void syscall_stub();
void initialize_syscalls(){
    idt_set_gate(0x80, (unsigned)syscall_stub, 0x08, 0xEE);//0x8E); //syscall
    for (int i = 0; i < MAX_SYSCALL_NUMBER; i++)
    {
        syscall_handlers[i] = unkown_syscall;
    }
    
}

int install_syscall_handler(uint32_t syscall_number, void (*syscall_handler)(struct regs *r))
{
    if(syscall_number >= MAX_SYSCALL_NUMBER){
        return -1; //out of index
    }

    syscall_handlers[syscall_number] = syscall_handler;
    return 0;
}


void syscall_handler(struct regs *r){
    if(r->eax >= MAX_SYSCALL_NUMBER){
        uart_print(COM1, "Syscall , %x, out of range!. Halting", r->eax);
        halt();
    }
    //ignore read for now
    if(r->eax){
        // fb_console_printf("calling syscall_handler %u\n", r->eax);

    } 
    syscall_handlers[r->eax](r);    
    return;
}

//syscalls defined here
void syscall_getpid(struct regs * r){
    r->eax = current_process->pid;
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
    // if(tar_get_filetype(current_process->open_descriptors[new_fd].f_inode) == FIFO_SPECIAL_FILE)
    //     // current_process->open_descriptors[new_fd].ops.open(
    //     //     current_process->open_descriptors[new_fd].f_mode == 1 ? 0 : 1,
    //     //     current_process->open_descriptors[new_fd].ops.priv
    //     //);

    r->eax = new_fd;
    return;
}


void syscall_execve(struct regs * r){ //more like spawn 
    r->eax = -1;
    if(tar_get_file((const char *)r->ebx, O_RDONLY) == NULL) return;
    
    //leap of faith
    pcb_t * np = create_process((char *)r->ebx, (char **)r->ecx); //pathname and arguments
    
    //copy file descriptors
    memcpy(np->open_descriptors, current_process->open_descriptors, MAX_OPEN_FILES * sizeof(file_t));
    for(int i = 0; i < MAX_OPEN_FILES; ++i){

        // if(np->open_descriptors[i].f_inode && tar_get_filetype(np->open_descriptors[i].f_inode) == FIFO_SPECIAL_FILE){
        //     np->open_descriptors[i].ops.open(
        //     np->open_descriptors[i].f_mode == 1 ? 0 : 1,
        //     np->open_descriptors[i].ops.priv
        // );
        // }

    }

    //copy the working directory of the caller
    kfree(np->cwd);
    np->cwd = kmalloc( strlen(current_process->cwd) + 1) ;
    memcpy(np->cwd, current_process->cwd, strlen(current_process->cwd) + 1 );


    np->parent = current_process->parent;
    np->childs = current_process->childs;
    np->pid = current_process->pid; //ughhh

    current_process->state = TASK_ZOMBIE;
    //force schedule?
    r->eax = 1;
    schedule(r);
    
    return;
}


void syscall_exit(struct regs *r){ 

    fb_console_printf("process %s called exit\n", current_process->filename);
    int exitcode = (int)r->ebx;
    r->eax = exitcode;
    current_process->state = TASK_ZOMBIE;


    schedule(r);
    
    


}




typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;
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



#include <semaphore.h>
#include <circular_buffer.h>
#include <pipe.h>
extern volatile int  is_received_char;
extern char ch;
extern volatile int  is_kbd_pressed;
extern char kb_ch;
extern uint8_t kbd_scancode;
extern semaphore_t * semaphore_uart_handler;
extern circular_buffer_t * ksock_buf;




typedef struct  {
    uint32_t d_ino;
    uint32_t d_off;      
    unsigned short d_reclen;    /* Length of this record */
    int  d_type;      /* Type of file; not supported*/
    char d_name[256]; /* Null-terminated filename */
} dirent_t;

void syscall_read(struct regs *r){ 

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
            for(size_t i = 0; i< count; ++ i){
                buf[i] = result[file->f_pos + i];
            }   

            file->f_pos += count;
            r->eax = count;


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
                ;
                int32_t mouse = ps2_mouse_read();
                if(mouse == -1){
                    r->eax = -1; break;
                }

                r->eax = 3; 
                memcpy(buf, &mouse, 3);

                break;
            case 2: // /dev/fb
                //it should be read only though
                // r->eax = ((uint8_t *)framebuffer)[file->f_pos];
                // file->f_pos += 1;
                break;
            case 3: //dev/kbd
                //here i should put raw data

                if(is_kbd_pressed == 1){
                    is_kbd_pressed = 0;
                    r->eax = kbd_scancode;
                    *buf = kbd_scancode;
                }
                else{
                    //actually block but
                    r->eax = 0; //read size of 0    
                }
                break;

            case 4: //tty character device

                switch (tar_get_minor_number(file->f_inode)){
                    case 0: //fb

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

                    case 1: //com1

                         if(!is_received_char){
                            
                            // semaphore_uart_handler->value--;
                            // queue_enqueue_item(&semaphore_uart_handler->queue, current_process);

                            // current_process->state = TASK_INTERRUPTIBLE;
                            // save_context(r, current_process);
                            
                            schedule(r);
                            
                        }
                        
                        // fb_console_printf("is it tho darling\n");
                        is_received_char = 0;
                        r->eax = ch;
    
                        
                        break;

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
        
        r->eax = current_process->open_descriptors[fd].ops.read(buf, 0, 1, current_process->open_descriptors[fd].ops.priv);
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
            r->eax = file->ops.write(buf, 0, count, file->ops.priv );
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

    // if( tar_get_filetype(process->open_descriptors[fd].f_inode) == FIFO_SPECIAL_FILE){
        
    //      current_process->open_descriptors[fd].ops.close(
    //             current_process->open_descriptors[fd].f_mode == 1 ? 0 : 1,
    //             current_process->open_descriptors[fd].ops.priv 
    //             );

    // }

    
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


typedef struct  {
        //    unsigned int   st_dev;      /* ID of device containing file */
        //    unsigned int   st_ino;      /* Inode number */
           unsigned int  st_mode;     /* File type and mode */
        //    unsigned int st_nlink;    /* Number of hard links */
           unsigned int   st_uid;      /* User ID of owner */
           unsigned int   st_gid;      /* Group ID of owner */

} stat_t;

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


 
void syscall_fork(struct regs * r){ //the show begins
    
    r->eax = -1;
    save_context(r, current_process); //save the latest context
    
    
    //create same process
    pcb_t* child = create_process( current_process->filename, current_process->argv);
    
    child->parent = current_process->self;
    list_insert_end(current_process->childs, child);

    
    list_t *clist;
    listnode_t * cnode;    


    list_t *list;
    listnode_t * node;    


    list = current_process->mem_mapping;
    node = list->head;

    clist = child->mem_mapping;
    cnode = clist->head;

    

    for(unsigned int i = 0; i < list->size  ; ++i){

        vmem_map_t * vm = node->val;

        if(!(vm->attributes >> 20)){
            //a free page no page to copy so just pass
            node = node->next;
            continue;
        }
        vmem_map_t * cm = kmalloc(sizeof(vmem_map_t));

        //create page for the child, set it to zero
        u8 * ch_page = kpalloc(1);
        memset(ch_page, 0, 4096);
        
        //create the same linked page list for the child
        cm->vmem = vm->vmem;
        cm->phymem = (uint32_t)get_physaddr( ch_page );
        vmm_mark_allocated(clist, cm->vmem, cm->phymem, cm->attributes);
        // list_insert_end(clist, cm);
        
        
        //mapping calling process phy page to memory window
        map_virtaddr(memory_window, (void *)vm->phymem, PAGE_PRESENT | PAGE_READ_WRITE); 
        memcpy(ch_page, memory_window, 4096); //copying calling process page to child's
        map_virtaddr_d(child->page_dir, ( void* )cm->vmem, ( void* )cm->phymem, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);

        //unmap the ch_page
        unmap_virtaddr(ch_page);
        kernel_heap -= 0x1000;

        //increment to the next node
        node = node->next;
    }



    child->argc = current_process->argc;
    // child->argv = current_process->argv; //nuh uh
    child->argv = kcalloc(child->argc, sizeof(char**));
    for(int i = 0; i < child->argc; ++i){
        child->argv[i] = kmalloc( strlen(current_process->argv[i]) + 1 );
        memcpy(child->argv[i], current_process->argv[i], strlen(current_process->argv[i]) + 1);
    }

    u32 old_cr3 = child->regs.cr3;
    child->regs = current_process->regs;
    child->regs.cr3 = old_cr3;

    
    //inherent file descriptors
    memcpy(child->open_descriptors, current_process->open_descriptors, MAX_OPEN_FILES * sizeof(file_t));
    //if there's a unnamed pipe i should increment the references as well
    for(int fd = 0; fd < MAX_OPEN_FILES; ++fd){
        
        // if( child->open_descriptors[fd].f_inode &&  tar_get_filetype(child->open_descriptors[fd].f_inode) == FIFO_SPECIAL_FILE){

        //     current_process->open_descriptors[fd].ops.open(
        //         current_process->open_descriptors[fd].f_mode == 1 ? 0 : 1,
        //         current_process->open_descriptors[fd].ops.priv 
        //         );
            
        // }
    }


    //inherent working directory;
    kfree(child->cwd);
    child->cwd = kmalloc( strlen(current_process->cwd) );
    memcpy(child->cwd, current_process->cwd, strlen(current_process->cwd));


    
    child->state = TASK_RUNNING;


    r->eax = child->pid;
    child->regs.eax = 0;

    schedule(r);
    
    return;
}



void syscall_pipe(struct regs * r){
    int fd[2] = {-1, -1};

    for(int f = 0; f < 2; f++){
        for(int i = 0; i < MAX_OPEN_FILES; i++){
            if(current_process->open_descriptors[i].f_inode == NULL){ //find empty entry
                current_process->open_descriptors[i].f_inode = (void *)1;
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

    file_t read_pipe  = pipe_create(64);
    file_t write_pipe = read_pipe;

    //read head
    current_process->open_descriptors[ fd[0] ] = read_pipe;
    current_process->open_descriptors[ fd[0] ].f_mode = O_RDONLY;
    
    //write head
    current_process->open_descriptors[ fd[1] ] = write_pipe;
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
   save_context(r, current_process);
   if(current_process->childs->size){ //do you have any darling?
    current_process->state = TASK_INTERRUPTIBLE;
   }

   schedule(r);

}


#define MAP_ANONYMOUS 1

//void *mmap(void *addr , size_t length, int prot, int flags, int fd, size_t offset)
void syscall_mmap(struct regs *r ){
    
    r->eax = 0;
    void  *addr = (void *)r->ebx;
    size_t length = (size_t)r->ecx;
    int prot = (int)r->edx;
    int flags = (int)r->esi;
    int fd = (int)r->edi;
    int offset = (int)r->ebp;


    if( fd == -1 && flags & MAP_ANONYMOUS){ // practically requesting a page
        void* page = allocate_physical_page();
        void * suitable_vaddr = vmm_get_empty_vpage(current_process->mem_mapping, 4096);
        
        map_virtaddr_d(current_process->page_dir, suitable_vaddr, page, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
        vmm_mark_allocated(current_process->mem_mapping, (u32)suitable_vaddr, (u32)page, VMM_ATTR_PHYSICAL_PAGE);
        memset(suitable_vaddr, 0, 4096);
        r->eax = (u32)suitable_vaddr;
        return;
    
    }
    else{

    // list_vmem_mapping(current_process);
    tar_header_t * device_in_question = current_process->open_descriptors[fd].f_inode;

    //for now only device files are mappable to the address space
    switch(tar_get_filetype(device_in_question)){

        case BLOCK_SPECIAL_FILE:
            //only support for block device is the fb
            if(tar_get_major_number(device_in_question) != 2){
                break;
            }

            vmem_map_t mp;
            // vmem_map_t mp = vmm_get_empty_vpage();  //*pf;
            mp.vmem = (u32)vmm_get_empty_vpage(current_process->mem_mapping, length);  //*pf;
            // fb_console_printf("->vmem:%x phymem:%x \n", pf->vmem, pf->phymem);
            
            uint8_t * fb_addr = get_framebuffer_address();
            int page_size = length / 4096 + (length % 4096 != 0); //ceil or somethin
            

            for(int i = 0; i < page_size; ++i){

                map_virtaddr_d(current_process->page_dir, (void *)(mp.vmem + i*0x1000), (void *)(fb_addr + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
                vmm_mark_allocated(current_process->mem_mapping, mp.vmem + i*0x1000, ((u32)fb_addr) + i*0x1000, VMM_ATTR_PHYSICAL_MMIO);
            }

            // list_vmem_mapping(current_process);
            r->eax = mp.vmem;
            return;
            
            break;
        default:
            fb_console_printf("files other than block devices are not supported");
            break;
    }

    }
    //err
    r->eax = -1;
    return;

}



void syscall_kill(struct regs *r ){
    pid_t pid = (pid_t)r->ebx;
    int sig   = (int)r->ecx;
    pcb_t * dest_proc = process_get_by_pid(pid);
    
    if(!dest_proc){
        fb_console_printf("Failed to find process by pid:%u \n", pid);
        r->eax = -1;
        return;
    }

    dest_proc->recv_signals = sig;
    r->eax = 0;
    return;

}



void syscall_getcwd(struct regs *r ){

    char* buf = (char*)r->ebx;
    size_t size = (size_t)r->ecx;

    if(size < strlen(current_process->cwd) )
    {

        r->eax =  0x80000000 | 0x00000001; //  a negative number to say that it is an error //ERANGE
        return;
    }

    //copy the thingy
    size = strlen(current_process->cwd);
    memcpy(buf, current_process->cwd, size + 1);
    
    r->eax = (u32)buf;
}

void syscall_chdir(struct regs *r ){
    const char* path = (const char *)r->ebx;


    //should canonize the path
    //but 
    
    kfree(current_process->cwd);
    current_process->cwd = kmalloc( strlen(path) + 1);
    memcpy(current_process->cwd, path, strlen(path) + 1);

    r->eax = 0;

}


typedef struct {

    long uptime;             /* Seconds since boot */
    unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    unsigned long totalram;  /* Total usable main memory size */
    unsigned long freeram;   /* Available memory size */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap;  /* Swap space still available */
    unsigned short procs;    /* Number of current processes */
    char _f[22];             /* Pads structure to 64 bytes */
} ksysinfo_t;


#include <pmm.h>
extern uint8_t * bitmap_start;
extern uint32_t bitmap_size;

void syscall_sysinfo(struct regs *r ){
    ksysinfo_t * info = (void *)r->ebx;

    //info can be looked wheter its valid or not
    if(!is_virtaddr_mapped_d(current_process->page_dir, info)){
        r->eax = -1;
        return;
    }

    memset(info, 0, sizeof(ksysinfo_t));
    info->procs = process_list->size;
    info->totalram = bitmap_size * 4096 * 8;

    //calculating the usage
    long free_pages = 0;
    for(unsigned int  i = 0; i < bitmap_size; ++i ){
        
        for(int bit = 0; bit < 8 ; ++bit){
            
            if(GET_BIT(bitmap_start[i], bit) == 0){ //empty page
                
                free_pages += 1;    
            }
        }
    }
    
    info->freeram = free_pages * 4096;
    
    r->eax = 0;
    return;
}