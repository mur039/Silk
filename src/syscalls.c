#include <syscalls.h>
#include <isr.h>
#include <pmm.h>
#include <vmm.h>
#include <process.h>
#include <ps2_mouse.h>
#include <filesystems/vfs.h>

#define MAX_SYSCALL_NUMBER 256
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

//int execve(const char * path, const char * argv[]){
void syscall_execve(struct regs * r){ //more like spawn 
    r->eax = -1;
    char* path = (const char*)r->ebx;
    char** argv = (const char**)r->ecx;

    if(!kopen(path, 0)){

        r->eax = -1;
        return;    
    }


    pcb_t* cp = current_process;

    //free the resources and allocate new ones
    strcpy(cp->filename, path);
    
    for(int i = 0; i < cp->argc  ; ++i){
        kfree(cp->argv[i]);
    }
    kfree(cp->argv);

    int argc;
    for(argc = 0;  argv[argc];++argc);

    cp->argc = argc;
    cp->argv = kcalloc(argc + 1, sizeof(char*));
    for(int i = 0; i < argc; ++i){
        cp->argv[i] = strdup(argv[i]);    
    }

    
    // cp->state = TASK_CREATED;
    // r->eax = 0;
    // schedule(r);
    // return;






    memset(&cp->regs, 0, sizeof(context_t));
    cp->regs.eflags = 0x206;
    cp->stack = (void*)0xC0000000;
    cp->regs.esp = (0xC0000000);
    cp->regs.ebp = 0; //for stack trace
    cp->regs.cs = (3 * 8) | 3;
    cp->regs.ds = (4 * 8) | 3;
    cp->regs.es = (4 * 8) | 3;
    cp->regs.fs = (4 * 8) | 3;
    cp->regs.gs = (4 * 8) | 3;
    cp->regs.ss = (4 * 8) | 3;;

    cp->regs.cr3 = (u32)get_physaddr(cp->page_dir);


    //open descriptors unless specified remain open
    cp->state = TASK_CREATED; //so that schedular can load the image


    

    for(listnode_t * vmem_node = cp->mem_mapping->head; vmem_node; vmem_node = vmem_node->next){
        
        vmem_map_t * vmap = vmem_node->val;
        

        if(vmap->attributes >> 20){
            //gotta free the allocated pages
            deallocate_physical_page( (void*)vmap->phymem );
            unmap_virtaddr_d(cp->page_dir, (void*)vmap->vmem);
            uart_print(COM1, "vmem: %x->%x :: %x\r\n", vmap->vmem, vmap->phymem, vmap->attributes >> 20);
        }
        list_remove(cp->mem_mapping, vmem_node);
        // kfree(vmem_node->val);

    }


    vmem_map_t* empty_pages = kcalloc(1, sizeof(vmem_map_t));
    empty_pages->vmem = 0;
    empty_pages->phymem = 0xc0000000;
    empty_pages->attributes = (VMM_ATTR_EMPTY_SECTION << 20) | (786432); //3G -> 786432 pages
    list_insert_end(cp->mem_mapping, empty_pages);

    r->eax = 0;

    context_switch_into_process(r, cp); //hacky
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
    int fd = (int)r->ebx;

    file_t * file = &current_process->open_descriptors[fd];

    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
        return;
    }


    fs_node_t * node = (void*)file->f_inode;

    if (!(
            node->flags & FS_BLOCKDEVICE ||
            node->flags & FS_FILE        ))
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
        new_pos = node->length - r->ecx;
    }

    node->offset = new_pos;
    file->f_pos = new_pos;
    r->eax = new_pos;

    return;

}



int32_t open_for_process(pcb_t * process,  char * path, int flags, int mode ){
     
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        if(process->open_descriptors[i].f_inode == NULL){ //find empty entry

            // process->open_descriptors[i].f_inode = tar_get_file(path, mode); //open file

                
            // const char* abs_path = vfs_canonicalize_path(current_process->cwd, path);
            // fb_console_printf("syscall_open : %s\n", abs_path);
            fs_node_t* file =  kopen(path, flags);

            if(!file){ //faile to find file
                
                if(flags & O_CREAT){ //creating specified then create the file
                    char* canonical_path = vfs_canonicalize_path(current_process->cwd, path);
                    int index = strlen(canonical_path);
                    //will go backward until i find a /
                    for( ; index >= 0 ;index-- ){
                        if(canonical_path[index] == '/'){
                            canonical_path[index] = '\0';
                            break;
                        }
                    }
                    
                    file = kopen(canonical_path, flags);
                    create_fs(file, path, mode);
                    
                    file = kopen(path, flags);
                    kfree(canonical_path);

                }
                else{
                    return -1;
                }
            }
            


            

                uart_print(COM1, "Succesfully opened file %s : %x\r\n", path, i);
                process->open_descriptors[i].f_inode = (void*)file; //forcefully
                process->open_descriptors[i].f_flags = 0x6969;
                process->open_descriptors[i].f_pos = 0;
                process->open_descriptors[i].f_mode = mode;
                return i;
            /*
            else{ //failed to find file
                
                return -1;
            }
            
            */
        }
    }
    return -1;

}

/*O_RDONLY, O_WRONLY, O_RDWR*/
void syscall_open(struct regs *r){
    char* path = (char*)r->ebx;
    int flags = (int)r->ecx;
    int mode = (int)r->edx;
    r->eax = open_for_process(current_process, path, flags, mode) ;
}



#include <semaphore.h>
#include <circular_buffer.h>
#include <pipe.h>
extern volatile int  is_received_char;
extern char ch;
extern volatile int  is_kbd_pressed;
extern char kb_ch;
extern uint8_t kbd_scancode;
extern circular_buffer_t * ksock_buf;




typedef struct dirent dirent_t;


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

    
    if(file->f_flags == 0x6969){

        fs_node_t * node = (void*)file->f_inode;
        int result = read_fs(node, file->f_pos, count, buf);

        if( result == -1){
            
            current_process->state = TASK_INTERRUPTIBLE;
            r->eip -= 2;
            save_context(r, current_process);
            schedule(r);
            return;
        }

        r->eax = result;
        file->f_pos += r->eax;
        return;
    }


#if 0
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
            memcpy(dir->name, &h->filename[1], 99);
            dir->type = tar_get_filetype(h);
        }
        
        break;

    case FIFO_SPECIAL_FILE: //for both named and unnamed pipes
        
        r->eax = current_process->open_descriptors[fd].ops.read(buf, 0, 1, current_process->open_descriptors[fd].ops.priv);
        break;
        
    default:
        r->eax = 0;
        break;
            
}

#endif

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

    if(file->f_flags == 0x6969){
        fs_node_t * node = (fs_node_t *)file->f_inode;
        r->eax = write_fs(node, file->f_pos, count, buf);
        file->f_pos += r->eax;
        return; 
    }

#if 0
     switch (tar_get_filetype(file->f_inode)){
        
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

#endif
    

    return;
}


int close_for_process(pcb_t * process, int fd){
    
    if(process->open_descriptors[fd].f_inode == NULL){ //trying to closed files that is not opened
        return -1;
    }

    
    fs_node_t * node = (fs_node_t*)process->open_descriptors[fd].f_inode;
    close_fs(node);    
    process->open_descriptors[fd].f_inode = NULL;



    process->open_descriptors[fd].f_flags = 0;
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
    int pid = r->ebx;
    stat_t * stat = (void *)r->ecx;
    file_t * file;

    file = &current_process->open_descriptors[pid];
    if(file->f_inode == NULL){   //check if its opened
        r->eax = -1;
        return;
    }

    fs_node_t* node = (fs_node_t*)file->f_inode;
    tar_header_t* thead = node->device;

    switch(node->flags){
        case FS_DIRECTORY:
            stat->st_mode = DIRECTORY;
            break;

        case FS_BLOCKDEVICE:
            stat->st_mode = BLOCK_SPECIAL_FILE;
            break;
        
        case FS_CHARDEVICE:
            stat->st_mode = CHARACTER_SPECIAL_FILE;
            break;
                
        case FS_FILE:
            stat->st_mode = REGULAR_FILE;
            break;

        default: break;
    }

    stat->st_mode <<= 16 ;
    stat->st_mode |= file->f_flags;
    stat->st_uid = node->uid;
    stat->st_gid = node->gid;
    r->eax = 0;
    return;
}


 
void syscall_fork(struct regs * r){ //the show begins 
    
    uart_print(COM1, "syscall_fork: called from %s\r\n", current_process->filename);
    r->eax = -1;
    save_context(r, current_process); //save the latest context
    
    //create pointers for  curreent and new process
    pcb_t* curr_proc = current_process;
    pcb_t* child_proc = create_process(curr_proc->filename, curr_proc->argv);

    
    //clone cwd
    kfree(child_proc->cwd);
    child_proc->cwd = kcalloc(strlen(curr_proc->cwd), 1);
    strcpy(child_proc->cwd, curr_proc->cwd);

    //copy filedescriptor table
    memcpy(child_proc->open_descriptors, current_process->open_descriptors, sizeof(file_t) * MAX_OPEN_FILES ) ;
        //maybe open them as well but

    //copy registers except cr3
    {
        uint32_t oldcr3 = child_proc->regs.cr3;
        child_proc->regs = curr_proc->regs; 
        child_proc->regs.cr3 = oldcr3;
    }

    //copy memory map
    for(listnode_t* vnode = curr_proc->mem_mapping->head; vnode; vnode = vnode->next){

        vmem_map_t* mm = vnode->val;
        if(mm->attributes >> 20){ //allocated
            // uart_print(COM1, "ALLOCATED : %x-> %x\r\n", mm->vmem, mm->phymem);
            uint8_t* empty_page = allocate_physical_page();
            map_virtaddr(memory_window, empty_page, PAGE_READ_WRITE | PAGE_PRESENT );
            memcpy(memory_window, (void *)mm->vmem, 0x1000);

            map_virtaddr_d(child_proc->page_dir, (void *)mm->vmem, empty_page, PAGE_READ_WRITE | PAGE_USER_SUPERVISOR  | PAGE_PRESENT);
            vmm_mark_allocated(child_proc->mem_mapping, (void*)mm->vmem, empty_page, VMM_ATTR_PHYSICAL_PAGE);
            
        }
        else{ //free
            // uart_print(COM1, "FREE : %x-> %x\r\n", mm->vmem, mm->phymem);
        }

    }

    //finallt add child to child list of the parent
    list_insert_end(curr_proc->childs, child_proc);
    child_proc->state = TASK_RUNNING;
    r->eax = child_proc->pid;
    child_proc->regs.eax = 0;
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

    // process->open_descriptors[i].f_inode = (void*)file; //forcefully

    fs_node_t* pipe = create_pipe(300);
    
    open_fs(pipe, 1, 0);
    open_fs(pipe, 0, 1);
    
    
    //read head
    current_process->open_descriptors[ fd[0] ].f_inode = (void*)pipe;
    current_process->open_descriptors[ fd[0] ].f_mode = O_RDONLY;
    current_process->open_descriptors[ fd[0] ].f_flags = 0x6969;
    
    //write head
    current_process->open_descriptors[ fd[1] ].f_inode = (void*)pipe;
    current_process->open_descriptors[ fd[1] ].f_mode = O_WRONLY;
    current_process->open_descriptors[ fd[1] ].f_flags = 0x6969;
    
    filds[0] = fd[0];
    filds[1] = fd[1];
    r->eax = 0;
    return;
}


//pid_t wait4(pid_t pid, int * wstatus, int options, struct rusage * rusage);
/*options*/

#define WNOHANG    0x00000001
#define WUNTRACED  0x00000002
#define WCONTINUED 0x00000004


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

    if(!options){
        if(current_process->childs->size){ //do you have any darling?
            current_process->state = TASK_INTERRUPTIBLE;
        }
        schedule(r);
   }

    
    if(options & WNOHANG){
        //check if anychild process exited

        for(listnode_t * cnode = current_process->childs->head; cnode ; cnode = cnode->next){
            pcb_t* cproc = cnode->val;
            if(cproc->state == TASK_ZOMBIE){
                pid_t retpid = cproc->pid;
                if(wstatus) *wstatus = cproc->regs.eax;

                terminate_process(cproc);
                process_release_sources(cproc);

                r->eax = retpid;
                list_remove(current_process->childs, cnode);
                return;
            }
        }
        r->eax = 0;

    }

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
    

    // list_vmem_mapping(current_process);
    if(fd == -1){
        //well then no device for you
        r->eax = -1;
        return;

    }
    fs_node_t * device_in_question = (fs_node_t *)current_process->open_descriptors[fd].f_inode;

    //for now only device files are mappable to the address space
    switch( device_in_question->flags ){

        case FS_BLOCKDEVICE:
            fb_console_printf("syscall_mmap: mapping block device: %s\n", device_in_question->name);
            //only support for block device is the fb
            // if(tar_get_major_number(device_in_question) != 2){
            //     break;
            // }

            // vmem_map_t mp;
            // // vmem_map_t mp = vmm_get_empty_vpage();  //*pf;
            // mp.vmem = (u32)vmm_get_empty_vpage(current_process->mem_mapping, length);  //*pf;
            // // fb_console_printf("->vmem:%x phymem:%x \n", pf->vmem, pf->phymem);
            
            // uint8_t * fb_addr = get_framebuffer_address();
            // int page_size = length / 4096 + (length % 4096 != 0); //ceil or somethin
            

            // for(int i = 0; i < page_size; ++i){

            //     map_virtaddr_d(current_process->page_dir, (void *)(mp.vmem + i*0x1000), (void *)(fb_addr + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
            //     vmm_mark_allocated(current_process->mem_mapping, mp.vmem + i*0x1000, ((u32)fb_addr) + i*0x1000, VMM_ATTR_PHYSICAL_MMIO);
            // }

            // // list_vmem_mapping(current_process);
            // r->eax = mp.vmem;
            // return;
            
            break;

        default:
            fb_console_printf("files other than block devices are not supported");
            break;
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


void syscall_getdents(struct regs *r ){
    int fd = (int)r->ebx;
    void* dirp = (void*)r->ecx;
    unsigned int count = (unsigned int)r->edx;

    file_t * file = &current_process->open_descriptors[fd];
    if(file->f_inode == NULL){   //check if its opened
        r->eax = -1;
        return;
    }


    if(count < sizeof(struct dirent)){

        r->eax = -1;
        return;
    }

    

    fs_node_t * node = file->f_inode;
    

    uint32_t old_offset = node->offset;
    struct dirent* val =  readdir_fs(node, node->offset);
    
    if(val){
        memcpy(dirp, val, sizeof(struct dirent));
        kfree(val);

        if(old_offset > node->offset){
            r->eax = 0;
        }
        else{
            r->eax = count;
        }

    }
    else{
        r->eax = -1;
    }

    
    return;

}

void syscall_chdir(struct regs *r ){
    const char* path = (const char *)r->ebx;

    //check whether abs_path exist on vfs
    
    fs_node_t* alleged_node = kopen(path, 0);

    if(alleged_node &&  alleged_node->flags & FS_DIRECTORY){
        
        char* abs_path = vfs_canonicalize_path(current_process->cwd, path);
        r->eax = 0;
        kfree(current_process->cwd);
        current_process->cwd = abs_path;
        strcat(current_process->cwd, "/");
        return;
    }
    else{
        
        r->eax = 1;
        return;
    }
    


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


/*
int mount(const char *source, const char *target,
          const char *filesystemtype, unsigned long mountflags,
          const void *_Nullable data);
*/

/*mount flags*/
#define  MS_REMOUNT    0x00000001;
#define  MS_BIND       0x00000002;
#define  MS_SHARED     0x00000004;
#define  MS_PRIVATE    0x00000008;
#define  MS_SLAVE      0x00000010;
#define  MS_UNBINDABLE 0x00000020;

#include <filesystems/tar.h>
#include <filesystems/tmpfs.h>
#include <filesystems/fat.h>
#include <filesystems/ext2.h>
void syscall_mount(struct regs *r){

    char *source = (const char *)r->ebx;
    char *target = (const char *)r->ecx;
    char *filesystemtype = (const char *)r->edx;
    unsigned long mountflags = (unsigned long)r->esi;
    void *data = (const void *)r->edi;

    fb_console_printf(
                        "syscall_mount :      \n"
                        "\tsource : %s        \n"
                        "\target : %s         \n"
                        "\tfilesystemtype: %s  \n"
                        "\tmountflags : %x    \n"
                        "\tdata : %x          \n"
                        ,
                        source,
                        target,
                        filesystemtype, 
                        mountflags,
                        data
        );


    if(!mountflags){ // create a new node

        if(!strcmp(source, "devfs")){
            
            vfs_mount(target, devfs_create());
            r->eax = 0;
            return;
        }
        else if(!strcmp(source, "tmpfs")){
            
            vfs_mount(target, tmpfs_install());
            r->eax = 0;
            return;
        }
        else{

            //a device!!!
            //chech whether the device exists
            fs_node_t* devnode = kopen(source, 0);
            device_t* dev =  dev_get_by_name(devnode->name);
            
            if(dev){
                if(dev->dev_type == DEVICE_BLOCK){
                    
                    if(!strcmp(filesystemtype, "tar")){
                        
                        uint32_t * priv = dev->priv;
		                size_t max_size = priv[0];
		                uint8_t* raw_block = (void*)priv[1];
                        fs_node_t* node = tar_node_create(raw_block, max_size);
                        node->device = raw_block;
                        vfs_mount(target, node);
                        r->eax = 0;
                        return;
                    }
                    if(!strcmp(filesystemtype, "fat")){
                        
                        fs_node_t* node = fat_node_create(dev);
                        vfs_mount(target, node);
                        r->eax = 0;
                        return;
                    }
                    if(!strcmp(filesystemtype, "ext2")){
                        
                        fs_node_t* node = ext2_node_create(dev);
                        if(node){
                            vfs_mount(target, node);
                            r->eax = 0;
                            return;
                        }
                        
                        r->eax = -3;
                        return;
                    }
                    else{
                        r->eax = -3;
                        return;
                    }
                }
                r->eax = -2;
                return;
            }
            
            r->eax = -1;
            return;
        
        }

        
    }



    r->eax = -1;
    return;
}


void syscall_unlink(struct regs * r){
    char* path = (char*)r->ebx;
    
    fs_node_t* file = kopen(path, O_RDONLY);
    if(file){
        unlink_fs(file, path);
    }

    return;
}


void syscall_mkdir(struct regs *r ){
    char* path = (char*)r->ebx;
    int mode = (int)r->ecx;

    r->eax = 0;
    fs_node_t* file = kopen(path, O_RDONLY);
    if(file){ //folder? already exists
        close_fs(file);
        r->eax = -1;
        return;
    }

    //create the directory
    char* cpath = vfs_canonicalize_path(current_process->cwd, path);

    fb_console_printf("cannonicalized path that is: %s\n", cpath);

    char* parent_dir;
    char* child_dir;

    
    for(int i = strlen(cpath); i >= 0 ; --i){
        
        if(cpath[i] == '/'){
            parent_dir = kcalloc(i, 1);
            memcpy(parent_dir, cpath, i);
            parent_dir[i] = '\0';

            child_dir = kcalloc( strlen(cpath) - i, 1);
            memcpy(child_dir, &cpath[i + 1], strlen(cpath) - i);
            break;
        }
    }

    fb_console_printf("parent_dir::%s - child_dir::%s\n", parent_dir, child_dir);
    
    //open parent node
    fs_node_t * parent_node = kopen(parent_dir, O_RDONLY);
    if(!parent_node)
        fb_console_printf("failed to open parent_dir::%s\n", parent_dir);    
    
    
    mkdir_fs(parent_node, child_dir, 0664);

    kfree(child_dir);
    kfree(parent_dir);
    kfree(cpath);
    return;







    // //get the parent directory
    // char* parent_dir = strdup(cpath);
    // char* head = parent_dir + strlen(parent_dir);
    // for( ; *head != '/'; head--);
    // *head = '\0';
    
    // fb_console_printf("syscall_mkdir: %s :: %s\n", &head[1], parent_dir);
    // fs_node_t* parent_node = kopen(parent_dir, O_RDONLY);
    
    // r->eax = -1;
    // if(parent_node){
    //     mkdir_fs(parent_node, &head[1], mode);
    //     r->eax = 0;
    // }   
    
    // fb_console_printf("failed to open parent directory\n");

    // kfree(parent_dir);
    // kfree(cpath);
    return;
}


