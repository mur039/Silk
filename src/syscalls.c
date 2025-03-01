#include <syscalls.h>
#include <isr.h>
#include <pmm.h>
#include <vmm.h>
#include <process.h>
#include <ps2_mouse.h>
#include <filesystems/vfs.h>

#define MAX_SYSCALL_NUMBER 256
void (*syscall_handlers[MAX_SYSCALL_NUMBER])(struct regs *r);

void unkown_syscall(struct regs *r)
{
    uart_print(COM1, "Unkown syscall %x\r\n", r->eax);
    r->eax = -1;
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
    // ignore read for now
    if (r->eax)
    {
        // fb_console_printf("calling syscall_handler %u\n", r->eax);
    }
    syscall_handlers[r->eax](r);
    return;
}

// syscalls defined here
void syscall_getpid(struct regs *r)
{
    r->eax = current_process->pid;
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

    fb_console_printf("SYSCALL_DUP2, old_fd new_fd : %u %u\n", old_fd, new_fd);
    // check whether given old_fd is valid
    if (!current_process->open_descriptors[old_fd].f_inode)
    { // emty one so err
        return;
    }

    // check if new_fd is open, is so close it
    if (current_process->open_descriptors[new_fd].f_inode)
    { // open file_desc
        close_for_process(current_process, new_fd);
    }

    current_process->open_descriptors[new_fd] = current_process->open_descriptors[old_fd];
    // also open the newfd as well so that if fs implement refcounting it reference counts again
    fs_node_t *node = current_process->open_descriptors[new_fd].f_inode;

    int read = (current_process->open_descriptors[new_fd].f_mode & O_RDONLY) != 0 | (current_process->open_descriptors[new_fd].f_mode & O_RDWR) != 0;
    int write = (current_process->open_descriptors[new_fd].f_mode & O_WRONLY) != 0 | (current_process->open_descriptors[new_fd].f_mode & O_RDWR) != 0;

    open_fs(node, read, write); // to incerement refcount possibly
    r->eax = new_fd;
    return;
}

// int execve(const char * path, const char * argv[]){
void syscall_execve(struct regs *r)
{ // more like spawn

    r->eax = -1;
    char *path = (const char *)r->ebx;
    char **argv = (const char **)r->ecx;

    if (!kopen(path, 0))
    {

        r->eax = -1;
        return;
    }

    pcb_t *cp = current_process;

    // free the resources and allocate new ones
    strcpy(cp->filename, path);

    for (int i = 0; i < cp->argc; ++i)
    {
        kfree(cp->argv[i]);
    }
    kfree(cp->argv);

    int argc;
    for (argc = 0; argv[argc]; ++argc)
        ;

    cp->argc = argc;
    cp->argv = kcalloc(argc + 1, sizeof(char *));
    for (int i = 0; i < argc; ++i)
    {
        cp->argv[i] = strdup(argv[i]);
    }

    // cp->state = TASK_CREATED;
    // r->eax = 0;
    // schedule(r);
    // return;

    memset(&cp->regs, 0, sizeof(context_t));
    cp->regs.eflags = 0x206;
    cp->stack = (void *)0xC0000000;
    cp->regs.esp = (0xC0000000);
    cp->regs.ebp = 0; // for stack trace
    cp->regs.cs = (3 * 8) | 3;
    cp->regs.ds = (4 * 8) | 3;
    cp->regs.es = (4 * 8) | 3;
    cp->regs.fs = (4 * 8) | 3;
    cp->regs.gs = (4 * 8) | 3;
    cp->regs.ss = (4 * 8) | 3;
    ;

    cp->regs.cr3 = (u32)get_physaddr(cp->page_dir);

    // open descriptors unless specified remain open
    cp->state = TASK_CREATED; // so that schedular can load the image

    for (listnode_t *vmem_node = cp->mem_mapping->head; vmem_node; vmem_node = vmem_node->next)
    {

        vmem_map_t *vmap = vmem_node->val;

        if (vmap->attributes >> 20)
        {
            // gotta free the allocated pages
            deallocate_physical_page((void *)vmap->phymem);
            unmap_virtaddr_d(cp->page_dir, (void *)vmap->vmem);
            uart_print(COM1, "vmem: %x->%x :: %x\r\n", vmap->vmem, vmap->phymem, vmap->attributes >> 20);
        }
        list_remove(cp->mem_mapping, vmem_node);
        // kfree(vmem_node->val);
    }

    vmem_map_t *empty_pages = kcalloc(1, sizeof(vmem_map_t));
    empty_pages->vmem = 0;
    empty_pages->phymem = 0xc0000000;
    empty_pages->attributes = (VMM_ATTR_EMPTY_SECTION << 20) | (786432); // 3G -> 786432 pages
    list_insert_end(cp->mem_mapping, empty_pages);

    r->eax = 0;

    context_switch_into_process(r, cp); // hacky
    schedule(r);
    return;
}

void syscall_exit(struct regs *r)
{

    // fb_console_printf("process %s called exit\n", current_process->filename);
    int exitcode = (int)r->ebx;
    r->eax = exitcode & 0xFF;
    current_process->state = TASK_ZOMBIE;
    schedule(r);
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

    file_t *file = &current_process->open_descriptors[fd];

    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
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

int32_t open_for_process(pcb_t *process, char *path, int flags, int mode)
{

    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (process->open_descriptors[i].f_inode == NULL)
        { // find empty entry

            // process->open_descriptors[i].f_inode = tar_get_file(path, mode); //open file

            // const char* abs_path = vfs_canonicalize_path(current_process->cwd, path);
            // fb_console_printf("syscall_open : %s\n", abs_path);
            fs_node_t *file = kopen(path, flags);

            if (!file)
            { // faile to find file

                if (flags & O_CREAT)
                { // creating specified then create the file
                    char *canonical_path = vfs_canonicalize_path(current_process->cwd, path);
                    int index = strlen(canonical_path);
                    // will go backward until i find a /
                    for (; index >= 0; index--)
                    {
                        if (canonical_path[index] == '/')
                        {
                            canonical_path[index] = '\0';
                            break;
                        }
                    }

                    file = kopen(canonical_path, flags);
                    if (!file)
                    {
                        fb_console_printf("open_for_process: failed to open parent folder: %s\n", canonical_path);
                        return -1;
                    }

                    create_fs(file, path, mode);

                    file = kopen(path, flags);
                    kfree(canonical_path);
                }
                else
                {
                    return -1;
                }
            }

            uart_print(COM1, "Succesfully opened file %s : %x\r\n", path, i);
            process->open_descriptors[i].f_inode = (void *)file; // forcefully
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
extern volatile int is_received_char;
extern char ch;
extern volatile int is_kbd_pressed;
extern char kb_ch;
extern uint8_t kbd_scancode;
extern circular_buffer_t *ksock_buf;

typedef struct dirent dirent_t;

void syscall_read(struct regs *r)
{

    int fd = (int)r->ebx;
    u8 *buf = (u8 *)r->ecx;
    size_t count = (size_t)r->edx;

    file_t *file = &current_process->open_descriptors[fd];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
        return;
    }

    if ((file->f_mode & (O_RDONLY | O_RDWR)) == 0)
    { // check if its readable
        r->eax = -1;
        return;
    }

    if (file->f_flags == 0x6969)
    {

        fs_node_t *node = (void *)file->f_inode;
        int result = read_fs(node, file->f_pos, count, buf);

        if (result == -1)
        {

            current_process->state = TASK_INTERRUPTIBLE;
            r->eip -= 2; // rewind back to the syscall
            save_context(r, current_process);
            schedule(r);
            return;
        }

        r->eax = result;
        file->f_pos += r->eax;
        return;
    }

    return;
}

void syscall_write(struct regs *r)
{

    int fd = (int)r->ebx;
    u8 *buf = (u8 *)r->ecx;
    size_t count = (size_t)r->edx;

    file_t *file;
    file = &current_process->open_descriptors[fd];

    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
        return;
    }

    if ((file->f_mode & (O_WRONLY | O_RDWR)) == 0)
    { // check if its writable
        r->eax = -1;
        return;
    }

    if (file->f_flags == 0x6969)
    {
        fs_node_t *node = (fs_node_t *)file->f_inode;
        r->eax = write_fs(node, file->f_pos, count, buf);
        file->f_pos += r->eax;
        return;
    }

    return;
}

int close_for_process(pcb_t *process, int fd)
{

    if (process->open_descriptors[fd].f_inode == NULL)
    { // trying to closed files that is not opened
        return -1;
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
    //    unsigned int   st_dev;      /* ID of device containing file */
    //    unsigned int   st_ino;      /* Inode number */
    unsigned int st_mode; /* File type and mode */
                          //    unsigned int st_nlink;    /* Number of hard links */
    unsigned int st_uid;  /* User ID of owner */
    unsigned int st_gid;  /* Group ID of owner */

} stat_t;

void syscall_fstat(struct regs *r)
{
    // ebx = fd, ecx = &stat
    int pid = r->ebx;
    stat_t *stat = (void *)r->ecx;
    file_t *file;

    file = &current_process->open_descriptors[pid];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
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
    stat->st_mode |= file->f_flags;
    stat->st_uid = node->uid;
    stat->st_gid = node->gid;
    r->eax = 0;
    return;
}

void syscall_fork(struct regs *r)
{ // the show begins

    uart_print(COM1, "syscall_fork: called from %s\r\n", current_process->filename);
    r->eax = -1;
    save_context(r, current_process); // save the latest context

    // create pointers for  curreent and new process
    pcb_t *curr_proc = current_process;
    pcb_t *child_proc = create_process(curr_proc->filename, curr_proc->argv);

    // clone cwd
    kfree(child_proc->cwd);
    child_proc->cwd = strdup(curr_proc->cwd);

    // copy filedescriptor table
    memcpy(child_proc->open_descriptors, current_process->open_descriptors, sizeof(file_t) * MAX_OPEN_FILES);

    // maybe open them as well but, yes use openfs to increment refcont for certain inodes...
    fb_console_put("In fork duplicating file descriptor\n");
    for (int i = 0; i < MAX_OPEN_FILES; ++i)
    {

        fs_node_t *node = (fs_node_t *)child_proc->open_descriptors[i].f_inode;
        if (!node)
        { // empty entry
            continue;
        }

        int read = (child_proc->open_descriptors[i].f_mode & O_RDONLY) != 0 || (child_proc->open_descriptors[i].f_mode & O_RDWR) != 0;
        int write = (child_proc->open_descriptors[i].f_mode & O_WRONLY) != 0 || (child_proc->open_descriptors[i].f_mode & O_RDWR) != 0;

        open_fs(node, read, write);
    }

    // copy registers except cr3
    {
        uint32_t oldcr3 = child_proc->regs.cr3;
        child_proc->regs = curr_proc->regs;
        child_proc->regs.cr3 = oldcr3;
    }

    // copy memory map
    for (listnode_t *vnode = curr_proc->mem_mapping->head; vnode; vnode = vnode->next)
    {

        vmem_map_t *mm = vnode->val;
        if (mm->attributes >> 20)
        { // allocated
            // uart_print(COM1, "ALLOCATED : %x-> %x\r\n", mm->vmem, mm->phymem);
            uint8_t *empty_page = allocate_physical_page();
            map_virtaddr(memory_window, empty_page, PAGE_READ_WRITE | PAGE_PRESENT);
            memcpy(memory_window, (void *)mm->vmem, 0x1000);

            map_virtaddr_d(child_proc->page_dir, (void *)mm->vmem, empty_page, PAGE_READ_WRITE | PAGE_USER_SUPERVISOR | PAGE_PRESENT);
            vmm_mark_allocated(child_proc->mem_mapping, (void *)mm->vmem, empty_page, VMM_ATTR_PHYSICAL_PAGE);
        }
        else
        { // free
            // uart_print(COM1, "FREE : %x-> %x\r\n", mm->vmem, mm->phymem);
        }
    }

    // finallt add child to child list of the parent
    list_insert_end(curr_proc->childs, child_proc);
    child_proc->state = TASK_RUNNING;
    r->eax = child_proc->pid;
    child_proc->regs.eax = 0;
    schedule(r);
    return;
}

void syscall_pipe(struct regs *r)
{
    int fd[2] = {-1, -1};

    for (int f = 0; f < 2; f++)
    {
        for (int i = 0; i < MAX_OPEN_FILES; i++)
        {
            if (current_process->open_descriptors[i].f_inode == NULL)
            { // find empty entry
                current_process->open_descriptors[i].f_inode = (void *)1;
                fd[f] = i;
                break;
            }
        }
    }

    if (fd[0] == -1 || fd[1] == -1)
    {
        r->eax = -1;
        return;
    }

    int *filds = (int *)r->ebx; // int fd[2]

    // process->open_descriptors[i].f_inode = (void*)file; //forcefully

    fs_node_t *pipe = create_pipe(300);

    open_fs(pipe, 1, 0);
    open_fs(pipe, 0, 1);

    // read head
    current_process->open_descriptors[fd[0]].f_inode = (void *)pipe;
    current_process->open_descriptors[fd[0]].f_mode = O_RDONLY;
    current_process->open_descriptors[fd[0]].f_flags = 0x6969;

    // write head
    current_process->open_descriptors[fd[1]].f_inode = (void *)pipe;
    current_process->open_descriptors[fd[1]].f_mode = O_WRONLY;
    current_process->open_descriptors[fd[1]].f_flags = 0x6969;

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

void syscall_wait4(struct regs *r)
{
    pid_t pid = (pid_t)r->ebx;
    int *wstatus = (int *)r->ecx;
    int options = (int)r->edx;
    // rusage is ignored for now

    /*
    for now its gonna be just wait for anything and in terminate process
    we will set status to running again when child get terminated
    */
    save_context(r, current_process);

    if (!options)
    {
        if (current_process->childs->size)
        { // do you have any darling?
            current_process->state = TASK_INTERRUPTIBLE;
        }
        schedule(r);
    }

    if (options & WNOHANG)
    {
        // check if anychild process exited

        for (listnode_t *cnode = current_process->childs->head; cnode; cnode = cnode->next)
        {
            pcb_t *cproc = cnode->val;
            if (cproc->state == TASK_ZOMBIE)
            {
                pid_t retpid = cproc->pid;
                if (wstatus)
                    *wstatus = cproc->regs.eax;

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

    if (fd == -1 && flags & MAP_ANONYMOUS)
    { // practically requesting a page
        void *page = allocate_physical_page();
        void *suitable_vaddr = vmm_get_empty_vpage(current_process->mem_mapping, 4096);

        map_virtaddr(suitable_vaddr, page, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
        vmm_mark_allocated(current_process->mem_mapping, (u32)suitable_vaddr, (u32)page, VMM_ATTR_PHYSICAL_PAGE);
        memset(suitable_vaddr, 0, 4096);
        r->eax = (u32)suitable_vaddr;
        return;
    }

    fs_node_t *device_in_question = (fs_node_t *)current_process->open_descriptors[fd].f_inode;

    // for now only device files are mappable to the address space
    switch (device_in_question->flags)
    {

        // if(device_in_question->)
    }

    // err
    r->eax = -1;
    return;
}

void syscall_kill(struct regs *r)
{
    pid_t pid = (pid_t)r->ebx;
    int sig = (int)r->ecx;
    pcb_t *dest_proc = process_get_by_pid(pid);

    if (!dest_proc)
    {
        fb_console_printf("Failed to find process by pid:%u \n", pid);
        r->eax = -1;
        return;
    }

    dest_proc->recv_signals = sig;
    r->eax = 0;
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

    file_t *file = &current_process->open_descriptors[fd];
    if (file->f_inode == NULL)
    { // check if its opened
        r->eax = -1;
        return;
    }

    if (count < sizeof(struct dirent))
    {

        r->eax = -1;
        return;
    }

    fs_node_t *node = file->f_inode;

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
    const char *path = (const char *)r->ebx;

    // check whether abs_path exist on vfs

    fs_node_t *alleged_node = kopen(path, 0);

    if (alleged_node && alleged_node->flags & FS_DIRECTORY)
    {

        char *abs_path = vfs_canonicalize_path(current_process->cwd, path); // allocates
        r->eax = 0;
        kfree(current_process->cwd);
        current_process->cwd = kcalloc(strlen(abs_path) + 1 + 1, 1);
        strcat(current_process->cwd, abs_path);
        strcat(current_process->cwd, "/");
        kfree(abs_path);
        return;
    }
    else
    {

        r->eax = 1;
        return;
    }
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
extern uint8_t *bitmap_start;
extern uint32_t bitmap_size;

void syscall_sysinfo(struct regs *r)
{
    ksysinfo_t *info = (void *)r->ebx;

    // info can be looked wheter its valid or not
    if (!is_virtaddr_mapped_d(current_process->page_dir, info))
    {
        r->eax = -1;
        return;
    }

    memset(info, 0, sizeof(ksysinfo_t));
    info->procs = process_list->size;
    info->totalram = bitmap_size * 4096 * 8;

    // calculating the usage
    long free_pages = 0;
    for (unsigned int i = 0; i < bitmap_size; ++i)
    {

        for (int bit = 0; bit < 8; ++bit)
        {

            if (GET_BIT(bitmap_start[i], bit) == 0)
            { // empty page

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
void syscall_mount(struct regs *r)
{

    char *source = (const char *)r->ebx;
    char *target = (const char *)r->ecx;
    char *filesystemtype = (const char *)r->edx;
    unsigned long mountflags = (unsigned long)r->esi;
    void *data = (const void *)r->edi;

    char *abs_source_path = vfs_canonicalize_path(current_process->cwd, source);
    char *abs_target_path = vfs_canonicalize_path(current_process->cwd, target);

    fb_console_printf(
        "syscall_mount :      \n"
        "\tsource : %s        \n"
        "\target : %s         \n"
        "\tfilesystemtype: %s  \n"
        "\tmountflags : %x    \n"
        "\tdata : %x          \n",
        source,
        target,
        filesystemtype,
        mountflags,
        data);

    if (!mountflags)
    { // create a new node

        if (!strcmp(source, "devfs"))
        {

            vfs_mount(abs_target_path, devfs_create());
            r->eax = 0;
            goto mount_end;
        }
        else if (!strcmp(source, "tmpfs"))
        {

            vfs_mount(abs_target_path, tmpfs_install());
            r->eax = 0;
            goto mount_end;
            ;
        }
        else
        {

            // a device!!!
            // chech whether the device exists
            fs_node_t *devnode = kopen(abs_source_path, 0);
            if (!devnode)
            {
                r->eax = -6;
                goto mount_end;
            }

            if (devnode->flags == FS_BLOCKDEVICE)
            {

                if (!strcmp(filesystemtype, "fat"))
                {

                    fs_node_t *node = fat_node_create(devnode);
                    if (node)
                    {
                        vfs_mount(abs_target_path, node);
                        r->eax = 0;
                    }
                    else
                    {
                        r->eax = -1;
                    }

                    goto mount_end;
                }
                if (!strcmp(filesystemtype, "ext2"))
                {

                    fs_node_t *node = ext2_node_create(devnode);
                    if (node)
                    {
                        vfs_mount(abs_target_path, node);
                        r->eax = 0;
                        goto mount_end;
                        ;
                    }

                    r->eax = -3;
                    goto mount_end;
                    ;
                }
                else
                {
                    r->eax = -3;
                    goto mount_end;
                    ;
                }
            }

            r->eax = -1;
            goto mount_end;
            ;
        }
    }

    r->eax = -1;

mount_end:
    kfree(abs_source_path);
    kfree(abs_target_path);
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
    int fd = r->ebx;
    unsigned long request = r->ecx;
    void *argp = r->edx;

    // ugly as fuck
    //  void * argp[4] = {
    //                      &r->edx,
    //                      &r->esi,
    //                      &r->edi,
    //                      &r->ebp
    //                  };
    if (fd < 0 || fd > MAX_OPEN_FILES)
    {
        r->eax = -1;
        return;
    }

    fs_node_t *node = current_process->open_descriptors[fd].f_inode;
    if (node->ioctl)
    {
        r->eax = node->ioctl(node, request, argp);
        return;
    }
    else
    {
        fb_console_printf("syscall_ioctl: doesn't have ioctl\n");
        r->eax = -1;
        return;
    }

    r->eax = 0;
}


void syscall_pivot_root(struct regs *r){

    //it will just iterate throught vfs fs_tree
    vfs_tree_traverse(fs_tree);
}
