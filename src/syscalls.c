#include <syscalls.h>
#include <isr.h>
#include <pmm.h>
#include <vmm.h>
#include <process.h>
#include <ps2_mouse.h>
#include <filesystems/vfs.h>

void (*syscall_handlers[MAX_SYSCALL_NUMBER])(struct regs *r);

void unkown_syscall(struct regs *r){
    r->eax = -ENOSYS;
}

extern void syscall_stub();
void initialize_syscalls()
{
    idt_set_gate(0x80, (unsigned)syscall_stub, 0x08, 0xEE); // 0x8E); //syscall
    for (int i = 0; i < MAX_SYSCALL_NUMBER; i++)
    {
        syscall_handlers[i] = unkown_syscall;
    }
}

int install_syscall_handler(uint32_t syscall_number, void (*syscall_handler)(struct regs *r))
{
    if (syscall_number >= MAX_SYSCALL_NUMBER)
    {
        return -1; // out of index
    }

    syscall_handlers[syscall_number] = syscall_handler;
    return 0;
}

void syscall_handler(struct regs *r)
{
    if (r->eax >= MAX_SYSCALL_NUMBER)
    {
        uart_print(COM1, "Syscall , %x, out of range!. Halting", r->eax);
        halt();
    }
    
    // fb_console_printf(
    //     "[*]syscall_handler:well pid:%u or filename:%s called syscall:%u\n", 
    //     current_process->pid, 
    //     current_process->filename, 
    //     r->eax
    // );
    
    syscall_handlers[r->eax](r);
    return;
}


void syscall_getpid(struct regs *r)
{
    r->eax = current_process->pid;
    return;
}

void syscall_getppid(struct regs *r){

    if(current_process->pid == 1){ //special case
        
        r->eax = 0;
    }
    else{
        
        r->eax = ((pcb_t*)current_process->parent->val)->pid;
    }
    return;
}



/*
    int dup2(int oldfd, int newfd);
*/
void syscall_dup2(struct regs *r)
{
    r->eax = -1; // err

    int old_fd = (int)r->ebx;
    int new_fd = (int)r->ecx;

    if(old_fd < 0 || old_fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    if(new_fd < 0 || new_fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t *oldfile, *newfile;
    oldfile = &current_process->open_descriptors[old_fd];
    newfile = &current_process->open_descriptors[new_fd];

    // check whether given old_fd is valid
    if (!oldfile->f_inode){ // emty one so err
        r->eax = -EBADF;
        return;
    }

    // check if new_fd is open, is so close it
    if (newfile->f_inode){
        close_for_process(current_process, new_fd);
    }

    current_process->open_descriptors[new_fd] = current_process->open_descriptors[old_fd];
    // also open the newfd as well so that if fs implement refcounting it reference counts again
    fs_node_t *node = (fs_node_t*)current_process->open_descriptors[new_fd].f_inode;

    int nflags = current_process->open_descriptors[new_fd].f_flags; 

    open_fs(node, nflags); // to incerement refcount possibly
    r->eax = new_fd;
    return;
}

// int execve(const char * path, const char * argv[]){
void syscall_execve(struct regs *r){

    r->eax = -1;
    char *path = ( char *)r->ebx;
    char **argv = ( char **)r->ecx;
    
    pcb_t *cp = current_process;
    fs_node_t* fnode = kopen(path, O_RDONLY);

    if (!fnode ){
        r->eax = -ENOENT;
        return;
    }

    //check if it's a file
    if(fnode->flags != FS_FILE){
        close_fs(fnode);
        r->eax = -EACCES;
        return;
    }


    //maybe check it here
    int type;
    uint32_t header;
    uint8_t* byteptr = (uint8_t*)&header;
    read_fs(fnode, 0, 4, byteptr);


    enum exec_type{
        ELF_FILE = 0,
        SHEBANG
    };


    if(!memcmp(byteptr, "#!", 2)){
        
        int i= 2;
        uint8_t ch = 0;
        while(ch != '\n' && i < 128){
            if( (int)read_fs(fnode, i++, 1, &ch) == -1){
                //it suppoesed to fine newline before eof
                close_fs(fnode);
                r->eax = -1;
                return;
            }
        }
        i--;
        if(ch != '\n'){ //line is greater than 128 and that's not acceptable
            
            close_fs(fnode);
            r->eax = -1;
            return;
        }

        char* interpreter_path = kcalloc((i-1), 1);
        read_fs(fnode, 2, i-2, interpreter_path);

        //remove post spaces as well
        for(int j = i-3; interpreter_path[j] == ' '; j--){
            interpreter_path[j] = '\0';
        }

        // fb_console_printf("syscall_execve: interpreter_path:%s\n", interpreter_path);

        type = SHEBANG;
        strcpy(cp->filename, interpreter_path);
        
        kfree(interpreter_path);
        close_fs(fnode);
    }
    else if(!memcmp(byteptr, "\x7f\x45\x4c\x46", 4) ){
        
        type = ELF_FILE;
        strcpy(cp->filename, path);
        close_fs(fnode);
    }
    else{
        close_fs(fnode);
        r->eax = -ENOEXEC;
        return;
    }


    //free the original argv
    for (int i = 0; i < cp->argc; ++i){
        kfree(cp->argv[i]);
    }
    kfree(cp->argv);


    if(type == ELF_FILE){
        
        int argc;
        for (argc = 0; argv[argc]; ++argc);
        
        cp->argc = argc;
        cp->argv = kcalloc(argc + 1, sizeof(char *));
        for (int i = 0; i < argc; ++i){
            cp->argv[i] = strdup(argv[i]);
        }

    }
    else{ 
        //shebang ise interpter -e filename and arguments
        //path give the filename
        //specified argument start in argv[1], so argc+1

        int argc;
        for (argc = 0 ; argv[argc]; ++argc);
        argc++;

        cp->argc = argc;
        cp->argv = kcalloc(argc + 1, sizeof(char *));
        cp->argv[0] = strdup(cp->filename);
        cp->argv[1] = strdup(path); //filename

        argv++;
        for (int i = 0; argv[i]; ++i){

            cp->argv[2 + i] = strdup(argv[i]);
        }

    }

    memset(&cp->regs, 0, sizeof(context_t));
    cp->regs.eflags = 0x206;
    cp->stack_top = (void *)0xC0000000;
    cp->stack_bottom = cp->stack_top - 4096;
    cp->regs.esp = (0xC0000000);
    cp->regs.ebp = 0; // for stack trace
    cp->regs.cs = (3 * 8) | 3;
    cp->regs.ds = (4 * 8) | 3;
    cp->regs.es = (4 * 8) | 3;
    cp->regs.fs = (4 * 8) | 3;
    cp->regs.gs = (4 * 8) | 3;
    cp->regs.ss = (4 * 8) | 3;
    cp->regs.cr3 = (u32)get_physaddr(cp->page_dir);
    
    cp->state = TASK_CREATED; // so that schedular can load the image
    
    pcb_t *proc = current_process;
    struct mm_struct* mm = proc->mm;
    while(1){
        
        //pop the vma as well
        struct vma* v = mm->mmap;
        if(!v){   
            break;
        }
        mm->mmap = v->next;
        
        int flags = v->flags >> 4;
        
        for(uintptr_t addr = v->start; addr < v->end; addr += 4096){
            
            if(!v->file || flags & VM_PRIVATE){

                if(v->flags & VM_GROWSDOWN){
                    if(!is_virtaddr_mapped((void*)addr)){
                        continue;
                    }
                }
                void* phy = get_physaddr((void*)addr);
                unmap_virtaddr((void*)addr);
                deallocate_physical_page(phy);
            }
        }

        kfree(v);
    }

    schedule(r);
    return;
}

void syscall_exit(struct regs *r)
{

    //also check for if pid 1 or 0 is trying to exit
    if(current_process->pid <= 1){
        fb_console_printf("Init process tried to exit, halting...\n");
        halt();
    }

    int exitcode = (int)r->ebx;
    current_process->exit_code = (exitcode & 0xFF) << 8;

    // fb_console_printf("[*]syscall_exit: process exited with exitcode: %u\n", exitcode);
    save_context(r, current_process);
    terminate_process(current_process);
    schedule(r);
    return;
}

typedef enum
{
    SEEK_SET = 0,
    SEEK_CUR = 1,
    SEEK_END = 2

} whence_t;

void syscall_lseek(struct regs *r)
{
    int fd = (int)r->ebx;
    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t *file = &current_process->open_descriptors[fd];

    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -EBADF;
        return;
    }

    fs_node_t *node = (void *)file->f_inode;

    if (!(
            node->flags & FS_BLOCKDEVICE ||
            node->flags & FS_FILE))
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

int32_t open_for_process(pcb_t *process, char *path, int flags, int mode){

    int fd = process_get_empty_fd(current_process);
    if(fd < 0){
        return -EMFILE;
    }

    file_t *filedesc = &process->open_descriptors[fd];

    fs_node_t *file = kopen(path, flags);
    if (!file){ 
                        
        if(!(flags & O_CREAT)){
            filedesc->f_inode = NULL;
            // fb_console_printf("sys_open: failed to find %s at %s", path, current_process->cwd);
            return -ENOENT; 
        }

        
        char *canonical_path = vfs_canonicalize_path(current_process->cwd, path);
        int index = strlen(canonical_path);
        // will go backward until i find a /
        for (; index >= 0; index--){
            if (canonical_path[index] == '/'){
                canonical_path[index] = '\0';
                break;
            }
        }

        //should recursively create folder?
        file = kopen(canonical_path, flags);
        if (!file){
            close_fs(file);
            kfree(canonical_path);
            filedesc->f_inode = NULL;
            return -ENOENT;;
        }

        create_fs(file, path, mode);
        file = kopen(path, flags);
        kfree(canonical_path);

    }

    if( (flags & O_TRUNC)  && (flags & (O_WRONLY | O_RDWR)) && file->ops.truncate){
        file->ops.truncate(file, 0);
    }
       
    filedesc->f_inode = (void *)file; // forcefully
    filedesc->f_flags = flags;
    filedesc->f_pos = 0;
    filedesc->f_mode = mode;
    return fd;

}




/*O_RDONLY, O_WRONLY, O_RDWR*/
void syscall_open(struct regs *r)
{
    char *path = (char *)r->ebx;
    int flags = (int)r->ecx;
    int mode = (int)r->edx;
    r->eax = open_for_process(current_process, path, flags, mode);
}

#include <semaphore.h>
#include <circular_buffer.h>
#include <pipe.h>
extern char ch;

typedef struct dirent dirent_t;

void syscall_read(struct regs *r){

    int fd = (int)r->ebx;
    u8 *buf = (u8 *)r->ecx;
    size_t count = (size_t)r->edx;

    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t *file = &current_process->open_descriptors[fd];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -EBADF;
        return;
    }

    if ((file->f_flags & (O_RDONLY | O_RDWR)) == 0)
    { // check if its readable
        r->eax = -EBADF;
        return;
    }

    if( !(is_virtaddr_mapped(buf) && (uint32_t)buf < 0xc0000000)){
        r->eax =  -EINVAL;
        return;
    }

    fs_node_t *node = (void *)file->f_inode;
    int result = read_fs(node, file->f_pos, count, buf);

    if (result == -1)
    {
        if (file->f_flags & O_NONBLOCK)
        {
            r->eax = -EAGAIN;
            return;
        }

        current_process->state = TASK_INTERRUPTIBLE;
        current_process->syscall_state = SYSCALL_STATE_PENDING;
        current_process->syscall_number = r->eax;

        save_context(r, current_process);
        schedule(r);
        return;
    }
    else if(result < 0){

        r->eax = result;
        return;
    }
    r->eax = result;
    file->f_pos += r->eax;
    return;
}

void syscall_write(struct regs *r)
{

    int fd = (int)r->ebx;
    u8 *buf = (u8 *)r->ecx;
    size_t count = (size_t)r->edx;

    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t *file;
    file = &current_process->open_descriptors[fd];

    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -EBADF;
        return;
    }

    if ((file->f_flags & (O_WRONLY | O_RDWR)) == 0  )
    { // check if its writable
        r->eax = -EINVAL;
        return;
    }

    fs_node_t *node = (fs_node_t *)file->f_inode;
    int result = write_fs(node, file->f_pos, count, buf);

    if (result == -1){

         if (file->f_flags & O_NONBLOCK)
        {
            r->eax = -EAGAIN;
            return;
        }

        current_process->state = TASK_INTERRUPTIBLE;
        current_process->syscall_state = SYSCALL_STATE_PENDING;
        current_process->syscall_number = r->eax;

        save_context(r, current_process);
        schedule(r);
        return;
    }

    r->eax = result;
    file->f_pos += r->eax;

    return;
}

int close_for_process(pcb_t *process, int fd)
{

    if(fd < 0 || fd > MAX_OPEN_FILES){
        return -EBADF;
    }

    if (process->open_descriptors[fd].f_inode == NULL)
    { // trying to closed files that is not opened
        return -EBADF;
    }

    fs_node_t *node = (fs_node_t *)process->open_descriptors[fd].f_inode;

    // node might have private data which if dynamically allocated will be free'd by the close_fs funciton but the node itself will be freed here
    close_fs(node);

    // why it causes tar.c readdir to go mad :( :( :( :(
    //  kfree(node); //wut?

    process->open_descriptors[fd].f_inode = NULL;

    process->open_descriptors[fd].f_flags = 0;
    process->open_descriptors[fd].f_mode = 0;
    process->open_descriptors[fd].f_pos = 0;
    return 0;
}

void syscall_close(struct regs *r)
{

    r->eax = close_for_process(current_process, (int)r->ebx);
    return;
}

typedef struct
{
    unsigned int   st_dev;      /* ID of device containing file */
    unsigned int   st_ino;      /* Inode number */
    unsigned int  st_mode;     /* File type and mode */
    unsigned int st_nlink;    /* Number of hard links */
    unsigned int   st_uid;      /* User ID of owner */
    unsigned int   st_gid;      /* Group ID of owner */

    unsigned int      st_rdev;     /* Device ID (if special file) */
    off_t      st_size;     /* Total size, in bytes */
    size_t  st_blksize;  /* Block size for filesystem I/O */
    size_t   st_blocks;   /* Number of 512 B blocks allocated */


    uint32_t  st_atime;  /* Time of last access */
    uint32_t  st_mtime;  /* Time of last modification */
    uint32_t  st_ctime;  /* Time of last status chiange */

} stat_t;

void syscall_fstat(struct regs *r)
{
    // ebx = fd, ecx = &stat
    int fd = r->ebx;
    stat_t *stat = (void *)r->ecx;
    file_t *file;

    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file = &current_process->open_descriptors[fd];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -EBADF;
        return;
    }

    fs_node_t *node = (fs_node_t *)file->f_inode;
    tar_header_t *thead = node->device;

    switch (node->flags)
    {
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

    default:
        break;
    }

    stat->st_mode <<= 16;
    stat->st_mode = node->flags;
    stat->st_mode |= file->f_flags;
    stat->st_uid = node->uid;
    stat->st_gid = node->gid;
    r->eax = 0;
    return;
}

void syscall_fork(struct regs *r) { // the show begins

    
    r->eax = -1;
    save_context(r, current_process); // save the latest context

    // create pointers for  curreent and new process
    pcb_t *curr_proc = current_process;
    pcb_t *child_proc = create_process(curr_proc->filename, curr_proc->argv);

    if(!child_proc){
        r->eax = -ENOMEM;
        return;
    }



    // clone cwd
    kfree(child_proc->cwd);
    child_proc->cwd = strdup(curr_proc->cwd);

    // copy filedescriptor table
    memcpy(child_proc->open_descriptors, current_process->open_descriptors, sizeof(file_t) * MAX_OPEN_FILES);

    // maybe open them as well but, yes use openfs to increment refcont for certain inodes...
    // fb_console_put("In fork duplicating file descriptor\n");
    file_t* file = child_proc->open_descriptors;
    for (int i = 0; i < MAX_OPEN_FILES; ++i)
    {
        fs_node_t *node = (fs_node_t *)file->f_inode;        
        if (!node) continue; // empty entry
    
        int rmask = O_RDONLY | O_RDWR, wmask = O_WRONLY | O_RDWR;
        open_fs(node, file->f_flags);
        file++;
    }

    // copy registers except cr3
    {
        uint32_t oldcr3 = child_proc->regs.cr3;
        child_proc->regs = curr_proc->regs;
        child_proc->regs.cr3 = oldcr3;
    }

    // copy memory map
    for (struct vma* v = curr_proc->mm->mmap; v; v = v->next){

        if (!v->file) { // non-file backed region
            //actually should i implement cow?
            if(v->flags & VM_GROWSDOWN){
                for(uintptr_t addr = v->start; addr < v->end; addr += 4096){
                    
                    //if not mapped
                    if(!is_virtaddr_mapped((void*)addr)){
                        continue;
                    }

                    uint8_t *empty_page = allocate_physical_page();
                    map_virtaddr(memory_window, empty_page, PAGE_READ_WRITE | PAGE_PRESENT);
                    memcpy(memory_window, (void *)addr, 0x1000);
                    map_virtaddr_d(child_proc->page_dir, (void *)addr, empty_page, get_virtaddr_flags( (void*)addr));
                }
            }
            else{
                for(uintptr_t addr = v->start; addr < v->end; addr += 4096){
                    
                    uint8_t *empty_page = allocate_physical_page();
                    map_virtaddr(memory_window, empty_page, PAGE_READ_WRITE | PAGE_PRESENT);
                    memcpy(memory_window, (void *)addr, 0x1000);
                    map_virtaddr_d(child_proc->page_dir, (void *)addr, empty_page, get_virtaddr_flags( (void*)addr));
                }
            }
        
            //insert it end of it
            struct vma* vc = kmalloc(sizeof(struct vma));
            *vc = *v;
            vmm_insert_vma(child_proc->mm, vc);   
        }
        else
        { 
            
        }
    }

    // finallt add child to child list of the parent and parent to the child
    list_insert_end(curr_proc->childs, child_proc);
    child_proc->parent = curr_proc->self;

    //inheret sid
    child_proc->sid = curr_proc->sid;
    child_proc->pgid = curr_proc->pgid;
    child_proc->ctty = curr_proc->ctty;
    child_proc->creds = curr_proc->creds; //credentials copied


    child_proc->state = TASK_RUNNING;
    r->eax = child_proc->pid;
    child_proc->regs.eax = 0;
    schedule(r);
    return;
}

void syscall_pipe(struct regs *r)
{
    int *filds = (int *)r->ebx; // int fd[2]

    int fd[2] = {-1, -1};
    
    fd[0] = process_get_empty_fd(current_process); //read
    if (fd[0] == -1){
        r->eax = -ENOBUFS;
        return;
    }


    fd[1] = process_get_empty_fd(current_process); //write
    if (fd[1] == -1){

        if(fd[0] > 0){
            current_process->open_descriptors[fd[0]].f_inode = NULL;
        }
        r->eax = -ENOBUFS;
        return;
    }
    

    file_t* fdesc_r = &current_process->open_descriptors[fd[0]];
    file_t* fdesc_w = &current_process->open_descriptors[fd[1]];
    
    fs_node_t *pipe_r = create_pipe(300);
    fs_node_t *pipe_w = memcpy( kmalloc(sizeof(fs_node_t)), pipe_r, sizeof(fs_node_t));

    open_fs(pipe_r, O_RDONLY);
    open_fs(pipe_w, O_WRONLY);

    // read head
    fdesc_r->f_inode = (void *)pipe_r;
    fdesc_r->f_mode = O_RDONLY;
    fdesc_r->f_flags = O_RDONLY;

    // write head
    fdesc_w->f_inode = (void *)pipe_w;
    fdesc_w->f_mode = O_WRONLY;
    fdesc_w->f_flags = O_WRONLY;

    filds[0] = fd[0];
    filds[1] = fd[1];
    r->eax = 0;
    return;
}

// pid_t wait4(pid_t pid, int * wstatus, int options, struct rusage * rusage);
/*options*/

#define WNOHANG 0x00000001
#define WUNTRACED 0x00000002
#define WCONTINUED 0x00000004

#define ECHILD 1
void syscall_wait4(struct regs *r)
{
    pid_t pid = (pid_t)r->ebx;
    int *wstatus = (int *)r->ecx;
    int options = (int)r->edx;
    // rusage is ignored for now

    
retry:
    //if process doesn't have any child then 
    if(!current_process->childs->size){
        r->eax = -ECHILD;
        return;
    }

    if(pid < -1){ //some precess group of abs value or something?
        r->eax = -EINVAL;
        return;
    }
    else if(pid == 0){ //any child process with equal process gruop id of the caller
        r->eax = -EINVAL;
        return;
    }
    else if(pid > 0){ //any child with same id;
        r->eax = -EINVAL;
        return;
    }
    else if(pid == -1){ //any child
        
        for (listnode_t *cnode = current_process->childs->head; cnode; cnode = cnode->next){
            pcb_t *cproc = cnode->val;

            if (cproc->state == TASK_ZOMBIE){ // yes exited fella
                pid_t retpid = cproc->pid;
                if (wstatus){
                    *wstatus = cproc->exit_code;
                }
                process_release_sources(cproc);


                r->eax = retpid;
                list_remove(current_process->childs, cnode);
                kfree(cnode);
                kfree(cproc);
                return;
            } 
            else if (cproc->state == TASK_STOPPED ){ // yes stopeed fella UNTRACED should be used as well but anyway
                
                if(!(options & WUNTRACED) || !cproc->exit_code) continue; //passover
                pid_t retpid = cproc->pid;
                if (wstatus){   
                    *wstatus = (cproc->exit_code << 8) | 0x7f;
                }
                cproc->exit_code = 0; //so on the next round we don't report it again
                r->eax = retpid;
                return;
            }

        }

        if(options & WNOHANG){ // no child has state change so don't block return mf
            r->eax = 0;
            return;
        }

        //hmmmmmm well we gonna block honey
        //the children with state change will wake my queue which i sleep on
        sleep_on(&current_process->waitqueue);
    }
    
    goto retry;
    
    

    
    //if not then we set syscall_state pending and schedule to another process    
    current_process->syscall_state = SYSCALL_STATE_PENDING;
    current_process->syscall_number = r->eax;
    schedule(r);
    return;
}

#define MAP_ANONYMOUS VM_PRIVATE

// void *mmap(void *addr , size_t length, int prot, int flags, int fd, size_t offset)
void syscall_mmap(struct regs *r)
{

    
    r->eax = 0;
    void *addr = (void *)r->ebx;
    size_t length = (size_t)r->ecx;
    int prot = (int)r->edx;
    int flags = (int)r->esi;
    int fd = (int)r->edi;
    int offset = (int)r->ebp;

    enable_interrupts();
    if (fd == -1 && flags & MAP_ANONYMOUS){ // practically requesting a page
        
        uint32_t nof_pages = (length / 4096) + (length % 4096 != 0);
        void** phypages = kcalloc(nof_pages, sizeof(void*));
        for(size_t i = 0; i < nof_pages; i++){
            void* addr = allocate_physical_page();
            if(!addr){
                //release the allocated pages then return -1;
                fb_console_printf("Out of physical memory\n");
                
                for(size_t j = 0; j < i; ++j){
                    deallocate_physical_page(phypages[j]);
                }
                
                kfree(phypages);
                r->eax = -1;
                return;
                
            }
            phypages[i] = addr;
        }

        uintptr_t base = (uintptr_t)vmm_get_empty_vpage(current_process->mm, (uintptr_t)addr, length);
        if((intptr_t)base < 0){
            r->eax = -EEXIST;
            return;
        }

        //allocate vma structure
        struct vma* v = kcalloc(1, sizeof(struct vma));
        if(!v){
            r->eax = -ENOMEM;
            return;
        }

        v->start = base;
        v->end = base + length;
        v->flags = (flags) | (prot & 0xf);
        v->file_offset = offset;
        v->file = NULL; //aka anon
    
        
        
        for(size_t i = 0; i < nof_pages; ++i){
            map_virtaddr((uint8_t*)v->start + i*4096 , phypages[i], PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
            memset((uint8_t*)v->start + i*4096, 0, 4096);
        }
        
        kfree(phypages);
        
        vmm_insert_vma(current_process->mm, v);
        r->eax = v->start;        
        return;
    }


    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t *file = process_get_file(current_process, fd);
    if(!file){
        r->eax = -EBADF;
        return;
    }

    fs_node_t *node = (fs_node_t *)file->f_inode;
    if(!node->ops.mmap){
        r->eax = -EINVAL;
        return;
    }

    uintptr_t base = (uintptr_t)vmm_get_empty_vpage(current_process->mm, (uintptr_t)addr, length);
    if((intptr_t)base < 0){
        r->eax = -EEXIST;
        return;
    }

    //allocate vma structure
    struct vma* v = kcalloc(1, sizeof(struct vma));
    if(!v){
        r->eax = -ENOMEM;
        return;
    }

    v->start = base;
    v->end = base + length;
    v->flags = (flags) | (prot & 0xf);
    v->file_offset = offset;
    v->file = node;
    //ops by device, next by mm_insert

    int ret =-ENODEV;
    if(node->ops.mmap)
        ret = node->ops.mmap(node, v);
    

    if(ret < 0){
        kfree(v);
        r->eax = ret;
        return;
    }
    vmm_insert_vma(current_process->mm, v);
    r->eax = v->start;
    return;
}

void syscall_kill(struct regs *r)
{
    pid_t pid = (pid_t)r->ebx;
    unsigned int signum = (int)r->ecx;
    save_context(r, current_process);

    int err = -EINVAL;

    if(pid <= 1){
        err = -ENOPERM;
        goto _kill_end;
    }
        
    err = process_send_signal(pid, signum);
    
    //if signal is sent to the caller itself then we should schedule
    if(current_process->pid == pid){
        schedule(r);
        return;
    }

    //if not return properly
_kill_end:
    r->eax = err;
    return;
}

void syscall_getcwd(struct regs *r)
{

    char *buf = (char *)r->ebx;
    size_t size = (size_t)r->ecx;

    if (size < strlen(current_process->cwd))
    {

        r->eax = 0x80000000 | 0x00000001; //  a negative number to say that it is an error //ERANGE
        return;
    }

    // copy the thingy
    size = strlen(current_process->cwd);
    memcpy(buf, current_process->cwd, size + 1);

    r->eax = (u32)buf;
}

void syscall_getdents(struct regs *r)
{
    int fd = (int)r->ebx;
    void *dirp = (void *)r->ecx;
    unsigned int count = (unsigned int)r->edx;

    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    file_t *file = &current_process->open_descriptors[fd];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -EBADF;
        return;
    }

    if (count < sizeof(struct dirent))
    {

        r->eax = -1;
        return;
    }

    fs_node_t *node = (fs_node_t *)file->f_inode;

    uint32_t old_offset = node->offset;
    struct dirent *val = readdir_fs(node, node->offset);

    if (val)
    {
        memcpy(dirp, val, sizeof(struct dirent));
        kfree(val);

        if (old_offset > node->offset)
        {
            r->eax = 0;
        }
        else
        {
            r->eax = count;
        }
    }
    else
    {
        r->eax = -1;
    }

    return;
}

void syscall_chdir(struct regs *r)
{
    
    
    char *path = (char *)r->ebx;
    
    // check whether abs_path exist on vfs
    fs_node_t *alleged_node = kopen(path, 0);
    r->eax = -ENOENT;
    if(!alleged_node){
        return;
    }

    if ( !(alleged_node->flags & (FS_DIRECTORY | FS_SYMLINK)) ){ //the node is neither a directory nor a symlink
    
        close_fs(alleged_node); //tmpfs releases this memory as well
        r->eax = -ENOTDIR;
        return;
    }

    close_fs(alleged_node); //tmpfs releases this memory as well

    char *abs_path = vfs_canonicalize_path(current_process->cwd, path); // allocates
    char *nbuffer = kmalloc(strlen(abs_path) + 2);
    sprintf(nbuffer, "%s/", abs_path);
    
    kfree(abs_path);
    kfree(current_process->cwd);
    current_process->cwd = nbuffer;

    r->eax = 0;
    return;
}

typedef struct
{

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
uint8_t *bitmap_start;
uint32_t bitmap_size;

#if 1
extern page_bitmap_t highmemory;
#endif

extern list_t* process_list;
void syscall_sysinfo(struct regs *r)
{

    ksysinfo_t *info = (void *)r->ebx;

    //check if it is a valid pointer
    r->eax = -1;
    if(!is_virtaddr_mapped(info) )
        return;

    
    bitmap_start = highmemory.bitmap;
    bitmap_size  = highmemory.bitmap_size;
    
    // info can be looked wheter its valid or not
    if (!is_virtaddr_mapped(info)){
        r->eax = -EINVAL;
        return;
    }

    memset(info, 0, sizeof(ksysinfo_t));
    info->procs = process_list->size;
    info->totalram = bitmap_size * 4096 * 8;

    // calculating the usage
    long free_pages = 0;
    for (unsigned int i = 0; i < bitmap_size; ++i)    {

        int octet = bitmap_start[i];
        if(!bitmap_start[i]){ free_pages += 8; }
        else{

            for (int bit = 0; bit < 8; ++bit){
                
                if( !(octet & 1)) free_pages++;
                octet >>= 1;
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
#define MS_REMOUNT 0x00000001;
#define MS_BIND 0x00000002;
#define MS_SHARED 0x00000004;
#define MS_PRIVATE 0x00000008;
#define MS_SLAVE 0x00000010;
#define MS_UNBINDABLE 0x00000020;

#include <filesystems/tar.h>
#include <filesystems/tmpfs.h>
#include <filesystems/fat.h>
#include <filesystems/ext2.h>
#include <filesystems/pts.h>
#include <filesystems/fs.h>

void syscall_mount(struct regs *r)
{
    
    char *source = (char *)r->ebx;
    char *target = (char *)r->ecx;
    char *filesystemtype = (char *)r->edx;
    unsigned long mountflags = (unsigned long)r->esi;
    void *data = (void *)r->edi;

    struct filesystem* fs = fs_get_by_name(filesystemtype);
    if(!fs){ //no such filesystem
        r->eax = -ENOENT;
        return;
    }

    if(mountflags){
        r->eax = -EINVAL;
        return;
    }

    fs_node_t* bdev = NULL;
    if(fs->fs_flags & FS_FLAGS_REQDEV){
        bdev = kopen(source, 0);
        if(!bdev){
            r->eax = -ENXIO;
            return;
        }

        if(bdev->flags != FS_BLOCKDEVICE){
            r->eax  -EINVAL;
            return;
        }
    }

    if(fs->probe && !fs->probe(bdev)){
        r->eax = -EINVAL;
        return;
    }

    fs_node_t* root = fs->mount(bdev, data);
    if(!root){
        r->eax = -EIO;
        return;
    }

    char* abs_target;
    if(!memcmp(target, "/", 2)){
        abs_target = strdup("/");
    }
    else{
        abs_target =  vfs_canonicalize_path(current_process->cwd, target);
    }

    vfs_mount(abs_target, root);
    kfree(abs_target);
    if(bdev) 
        close_fs(bdev);
    r->eax = 0;

    return;
}

// can't remove folder with it so
void syscall_unlink(struct regs *r)
{
    char *path = (char *)r->ebx;

    // well we should check the existance of the file
    fs_node_t *file = kopen(path, O_RDONLY);
    if (!file)
    {
        r->eax = -1;
        return;
    }

    if (file->flags & FS_DIRECTORY)
    {
        r->eax = -2;
        return;
    }
    close_fs(file); // at least of tmpfs it should deallocate in close function
    char *_1 = vfs_canonicalize_path(current_process->cwd, path);
    char *parentpath = vfs_canonicalize_path(_1, "./../");

    fb_console_printf("_1:%s <-> parentpath:%s\n", _1, parentpath);
    kfree(_1);

    // open parent folder
    fs_node_t *parent = kopen(parentpath, O_RDONLY);
    if (!parent)
    {

        r->eax = -3;
        return;
    }

    unlink_fs(parent, path);

    r->eax = 0;
    return;
}

void syscall_mkdir(struct regs *r)
{
    char *path = (char *)r->ebx;
    int mode = (int)r->ecx;

    r->eax = 0;
    fs_node_t *file = kopen(path, O_RDONLY);
    if (file)
    { // folder? already exists
        close_fs(file);
        r->eax = -1;
        return;
    }

    // create the directory
    char *cpath = vfs_canonicalize_path(current_process->cwd, path);

    fb_console_printf("cannonicalized path that is: %s\n", cpath);

    char *parent_dir;
    char *child_dir;

    for (int i = strlen(cpath); i >= 0; --i)
    {

        if (cpath[i] == '/')
        {
            parent_dir = kcalloc(i + 1, 1);
            memcpy(parent_dir, cpath, i);

            child_dir = kcalloc(strlen(cpath) - i, 1);
            memcpy(child_dir, &cpath[i + 1], strlen(cpath) - i);
            break;
        }
    }

    fb_console_printf("parent_dir::%s - child_dir::%s\n", parent_dir, child_dir);

    // open parent node
    fs_node_t *parent_node = kopen(parent_dir, O_RDONLY);

    if (!parent_node)
    {
        fb_console_printf("failed to open parent_dir::%s\n", parent_dir);
        r->eax = -1;
        return;
    }

    mkdir_fs(parent_node, child_dir, 0664);

    kfree(child_dir);
    kfree(parent_dir);
    kfree(cpath);
    return;
}

void syscall_ioctl(struct regs *r)
{
    int fd = (int)r->ebx;
    unsigned long request = (unsigned long)r->ecx;
    void *argp = (void*)r->edx;

    // ugly as fuck
    //  void * argp[4] = {
    //                      &r->edx,
    //                      &r->esi,
    //                      &r->edi,
    //                      &r->ebp
    //                  };

    //check fd numerically
    if (fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    //check if the file is an open file
    file_t* file = &current_process->open_descriptors[fd];
    if(!file->f_inode){
        r->eax = -EBADF;
        return;
    }


    fs_node_t *node = (fs_node_t *)file->f_inode;

    r->eax = ioctl_fs(node, request, argp);
    return;
}


void syscall_pivot_root(struct regs *r){

    //it will just iterate throught vfs fs_tree
    vfs_tree_traverse(fs_tree);
}




//int fcntl(int fd, int op, ... /* arg */ );
void syscall_fcntl(struct regs *r){ //max  6 arg so

    int fd = r->ebx;
    int op = r->ecx;

    int arg0 = r->edx;
    int arg1 = r->esi;
    int arg2 = r->edi;
    int arg3 = r->ebp;

    //check whether that fd is valid
    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return;
    }

    //as well as if it's opened
    if(!current_process->open_descriptors[fd].f_inode){
        r->eax = -EBADF;
        return;
    }

    //afterwards we can check op
    enum ops {
        F_GETFL = 0,
        F_SETFL,
        F_GETFD, 
        F_SETFD,
        F_DUPFD,

        OPS_MAX
    };

    if(op < 0 || op >= OPS_MAX){
        //invalid op
        r->eax = -EINVAL;
        return;
    }

    file_t* file = &current_process->open_descriptors[fd];
    switch(op){
        case F_GETFL:
            // fb_console_printf("[*]syscall_fcntl: on fd: %u F_GETFL operation is called, arg0: %x\n", fd, arg0);
            r->eax = file->f_flags; 
            return;
            break;

        case F_SETFL:
            // fb_console_printf("[*]syscall_fcntl: on fd: %u F_SETFL operation is called, arg0: %x\n", fd, arg0);
            file->f_flags = arg0;
            r->eax = 0;
            return;
            break;
        
        case F_DUPFD:
            // fb_console_printf("[*]syscall_fcntl: on fd: %u F_DUPFD operation is called, arg0: %x\n", fd, arg0);
        ;
            int minfd = arg0;
            int canditate = -1;

            for(int i = minfd; i < MAX_OPEN_FILES; ++i){

                if(!current_process->open_descriptors[i].f_inode){ //is it empty?
                    canditate = i;
                    break;
                }
            }

            if(canditate == -1){
                r->eax = -EMFILE;
                return;
            }

            file_t* dup = &current_process->open_descriptors[canditate];

            *dup = *file;

            r->eax = canditate;
            return;
        break;
    }
        

}

void syscall_pause(struct regs *r){
    
    //pauses current ttask until a signal is received thus return EINTR
    save_context(r , current_process);

    current_process->state = TASK_INTERRUPTIBLE;
    current_process->syscall_number = r->eax;
    current_process->syscall_state = SYSCALL_STATE_PENDING;

    schedule(r);
    return;
}


/*pid_t getpgrp(void);*/
void syscall_getpgrp(struct regs *r){
    r->eax = current_process->pgid;
    return;
}

/* int setpgid(pid_t pid, pid_t pgid); */
void syscall_setpgid(struct regs *r){
    
    pid_t pid = (pid_t)r->ebx;
    pid_t pgid = (pid_t)r->ecx;

    if(pgid < 0){
        r->eax = -EINVAL;
        return;
    }

    pcb_t* target_proc = NULL;
    if(pid == 0){
        target_proc = current_process;
    }
    else{
        target_proc = process_get_by_pid(pid);
        if(!target_proc){
            r->eax = -EINVAL;
            return;
        }

        //check if it's our child
        if( ((pcb_t*)target_proc->parent->val) != current_process){
            r->eax = -EINVAL;
            return;
        }
    }

    //check if callers session same as the target_proc 

    if(current_process->sid != target_proc->sid){
        r->eax = -EINVAL;
        return;
    }

    target_proc->pgid = pgid;

    r->eax = 0;
    return;
}

// pid_t setsid(void);
void syscall_setsid(struct regs* r){

    pcb_t* proc = current_process;
    
    if(proc->pid == proc->sid){
        r->eax = -ENOPERM;
        return;
    }

    proc->sid = proc->pid;
    proc->pgid = proc->pid;

    proc->ctty = NULL;
    r->eax = proc->pid;
    return;
}

typedef long time_t;
struct timespec {
    time_t tv_sec;   // seconds
    long   tv_nsec;  // nanoseconds (0 to 999,999,999)
};

void syscall_nanosleep(struct regs* r){

    const struct timespec* req = (const void*)r->ebx;
    struct timespec* rem = (void*)r->ecx;

    if(!req || !is_virtaddr_mapped((void*)req)){
        r->eax = -EFAULT;
    }

    if(req->tv_nsec < 0 || req->tv_nsec > 999999999){
        r->eax = -EINVAL;
    }

    // //since my timer has resolution of a ms
    size_t ms_ticks = (req->tv_sec * 1000) + (req->tv_nsec / 1000000);

    list_t l = list_create();
    current_process->itimer = timer_register(ms_ticks, 0, NULL, NULL);
    timer_event_t* timer = timer_get_by_index(current_process->itimer);

    sleep_on(&timer->wait_queue);

    current_process->itimer = -1;
    r->eax = 0;
    return;

}


void syscall_sched_yield(struct regs* r){
    save_context(r, current_process);
    schedule(r);
}


void poll_qproc(struct fs_node* n, list_t *wq, struct poll_table *pt) {
    // Add current task to this wait queue
    if(!list_find_by_val(wq, current_process)){
        list_insert_end(wq, current_process);
    }
}


void poll_wait(struct fs_node* f, list_t *wq, struct poll_table *pt) {
    if (pt && pt->qproc)
        pt->qproc(f, wq, pt);
}

void syscall_poll(struct regs* r){
    
    struct pollfd *fds = (struct pollfd *)r->ebx;
    size_t nfds = (size_t)r->ecx;
    int timeout = (int)r->edx; //in miliseconds perfect

    struct poll_table pt;
    pt.qproc = poll_qproc;


    for(;;){
        
        int ready = 0;
        for(size_t i = 0; i < nfds; ++i){
            
            file_t* file = get_file(fds[i].fd);
            if(!file){
                fds[i].revents = POLLNVAL;
                continue;
            }
            
            fs_node_t* fnode = (fs_node_t*)file->f_inode;
            if(!fnode->ops.poll){
                fds[i].revents = POLLIN | POLLOUT;
                ready++;
                continue;
            }
            
            short mask = fnode->ops.poll(fnode, &pt);
            fds[i].revents = mask & fds[i].events;
            if(fds[i].revents) ready++;
        }

        if(ready > 0){
            r->eax = ready;
            break;
        }

        if(timeout < 0){ //block indefinetly
            current_process->state = TASK_INTERRUPTIBLE;
            _schedule();
        }
        else if(timeout == 0){ //non_bocking
            r->eax = ready;
            break;
        }
        else{

            int t = timer_register(timeout, 0, NULL, NULL);
            timer_event_t* timer = timer_get_by_index(t);
            sleep_on(&timer->wait_queue);

            //if i wakeup early then
            if(timer_is_pending(timer))
                timer_destroy_timer(timer);

            timeout = 0;
        }        
    }
    
    return;
}

void syscall_getuid(struct regs* r){
    r->eax = current_process->creds.ruid;
    return;
}
void syscall_getgid(struct regs* r){
    r->eax = current_process->creds.rgid;
    return;
}

void syscall_geteuid(struct regs* r){
    r->eax = current_process->creds.euid;
    return;
}
void syscall_getegid(struct regs* r){
    r->eax = current_process->creds.egid;
    return;
}

void syscall_setuid(struct regs* r){
    
    uid_t uid = r->ebx;
    if(current_process->creds.euid == 0){ //root
        current_process->creds.ruid = uid;
        current_process->creds.euid = uid;
        current_process->creds.suid = uid;
        r->eax = 0;
        return;
    }
    else if(current_process->creds.ruid == uid){
        current_process->creds.euid = uid;
        r->eax = 0;
        return;
    }
    
    //no permission
    r->eax = -ENOPERM;
    return;
}
void syscall_setgid(struct regs* r){
    r->eax = current_process->creds.ruid;
    return;
}

void syscall_ftruncate(struct regs* r){
    
    int fd = (int)r->ebx;
    off_t length = (off_t)r->ecx;

    if(fd < 0 || fd > MAX_OPEN_FILES){
        r->eax = -EBADF;
        return ;
    }

    file_t* file = get_file(fd);
    if(!file){
        r->eax = -EBADF;
        return; 
    }

    fs_node_t* node = (fs_node_t*)file->f_inode;
    if(node->flags != FS_FILE || !(file->f_flags & (O_WRONLY | O_RDWR))){
        r->eax = -EINVAL;
        return;
    }   

    if(length < 0){
        r->eax = -EINVAL;
        return;
    }

    struct fs_ops* ops = &node->ops;
    if(!ops->truncate){
        r->eax = -EINVAL;
        return;
    }

    ops->truncate(node, length);
    r->eax = 0;
    return;
}