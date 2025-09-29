#include <process.h>
#include <elf.h>
#include <v86.h>

#include <fb.h>
#include <gdt.h>
#include <queue.h>
#include <syscalls.h>
#include <fpu.h>

#include <ptrace.h>
#include <filesystems/proc.h>

queue_t ready_queue;
queue_t blocked_queue;

list_t * process_list;
pid_t curr_pid ;
pcb_t * _current_process;

pcb_t* task0;

static inline pcb_t* next_process(pcb_t * cp){
    pcb_t * ret_val;
    if(!cp->self) return (pcb_t *)process_list->head->val;

    if(cp->self->next){ // no next?
        ret_val = cp->self->next->val;
    }
    else{
        if(!process_list->head){ //no programs_
            schedule(NULL);
        }
        ret_val = process_list->head->val;
    }
    return ret_val;
}   

pcb_t* create_idle_task();
void process_init() {
    //is just a double linked list bu its gonna be circular
    process_list = kcalloc(sizeof(list_t), 1);
    process_list->size = 0;
    process_list->head = 0;
    process_list->tail = 0;
    _current_process = NULL;
    curr_pid = 1;

    ready_queue = queue_create(-1); //no limit
    blocked_queue = queue_create(-1); //no limit
    // timer_register(10, 10, ( void (*)(void*))schedule, NULL);
    
    task0 = create_idle_task();

    ptrace_initialize();
    return;
}


int newschedule(struct regs * r){
    fb_console_printf("How the fuck i get here?\n");
    if(!_current_process){
        _current_process = queue_dequeue_item(&ready_queue);   
    }

    else{ //readl thing
        save_context(r, _current_process); //obv
        _current_process = queue_dequeue_item(&ready_queue);
        if(!_current_process){ //no bitches?
            idle();
        }


    }

    context_switch_into_process(r, _current_process);
    return 0;
}

extern void ktty();
void schedule(struct regs * r){
    

    if(process_list->size == 0){
        fb_console_printf("No processes, kernel panics :(\n");
        uart_print(COM1, "No processes, kernel panics :(\r\n");
        // ktty();
        halt();
    }
    
    int first_time = 0;
    if(_current_process == NULL){
        first_time = 1;
        _current_process = process_list->head->val;
    }


    switch(_current_process->state){
        case TASK_CREATED:
            ;
            //hacky but
            if(_current_process->regs.eflags & (1 << V86_VM_BIT)){ //v8086 proc
                _current_process->state = TASK_RUNNING;
                break;
            }
            load_process(_current_process);
            break;

        case TASK_RUNNING:
            if( !first_time ){

                save_context(r, _current_process);
            }
            break;
        
        case TASK_LOADING:
            break;
        
        case TASK_INTERRUPTIBLE: 
            save_context(r, _current_process);
            break;

    }

    // _current_process = next_process(_current_process);

    pcb_t* candinate = NULL;
    for(unsigned int i = 0; i < process_list->size; ++i){
        _current_process = next_process(_current_process);
        if(_current_process->state == TASK_RUNNING){
            candinate = _current_process;
            break;
        }
    
    }

    if(!candinate){

        // if(_current_process == task0) return;
        _current_process = task0;
    }

    context_switch_into_process(r, _current_process);

    __builtin_unreachable();
    return;
}



pid_t allocate_pid(){
    return curr_pid++;
}

pcb_t * create_process(char * filename, char **_argv) {

    // Create and insert a process, the pcb struct is in kernel space
    pcb_t * p1 = kcalloc(1, sizeof(pcb_t));
    if(!p1){
        fb_console_printf("failed to allocate mem\n");
        return NULL;
    }

    // //passing the arguments obv
    for(p1->argc = 0 ; _argv[p1->argc] ; p1->argc++);
    p1->argv = kcalloc( p1->argc + 1, sizeof(char *)); //must be null terminated

    for(int i = 0; i < p1->argc; ++i){
        
        p1->argv[i] = strdup(_argv[i]);
    }


    p1->pid = allocate_pid();
    p1->regs.eip = 0; //schedular will call load_process
    p1->regs.eflags = 0x206; // enable interrupts and other flags
    memcpy(p1->filename, filename, strlen(filename));

    p1->self = list_insert_end(process_list, p1);
    

    // 4kb initial stack
    p1->stack_top = (void*)0xC0000000;
    p1->stack_bottom = p1->stack_top - (4096 * 8);
    p1->regs.esp = (0xC0000000);
    p1->regs.ebp = 0; //for stack trace

    p1->regs.cs = (3 * 8) | 3;
    p1->regs.ds = (4 * 8) | 3;
    p1->regs.es = (4 * 8) | 3;
    p1->regs.fs = (4 * 8) | 3;
    p1->regs.gs = (4 * 8) | 3;
    p1->regs.ss = (4 * 8) | 3;;

    p1->mm = kcalloc(1, sizeof(struct mm_struct));
    p1->mm->mmap = NULL;
    p1->childs = kcalloc(1, sizeof(list_t));

 
    // Create an address space for the process, how ?
    // allocate a page directory for the process, then copy the entire kernel page dirs and tables(the frames don't have to be copied though)

    p1->page_dir = kpalloc(1);//allocate_physical_page();
    p1->regs.cr3 = (u32)get_physaddr(p1->page_dir);

    //map_virtaddr(p1->page_dir, p1->page_dir, PAGE_PRESENT | PAGE_READ_WRITE);
    memset(p1->page_dir, 0, 4096);
    memcpy(&p1->page_dir[768], &kdir_entry[768], (1023 - 768) * 4); //mapping //except last, recursion
    
    p1->page_dir[RECURSIVE_PD_INDEX] = p1->regs.cr3;
    p1->page_dir[RECURSIVE_PD_INDEX] |= (PAGE_PRESENT | PAGE_READ_WRITE);
    
    p1->recv_signals = 0;
    p1->state = TASK_CREATED;

    // p1->kstack = allocate_physical_page(); // pointer to its page
    void * page = kpalloc(1);
    p1->kstack = page;
    memset(page, 0, 4096);
    
    p1->cwd = kmalloc(2);
    if(!p1->cwd)
        error("failed to allocate for cwd");
    memcpy(p1->cwd, "/", 2);

    p1->syscall_state =  SYSCALL_STATE_NONE;
    p1->syscall_number = 0;
    
    p1->itimer = -1;


    //initialize the fpu as well
    initialize_fpu_user(p1);

    //register the proc as well
    process_create_proc_entry(p1);
    return p1;

}

pcb_t * load_process(pcb_t * proc){


    fs_node_t * exec_file = kopen(proc->filename, O_RDONLY);

    if(!exec_file){
        uart_print(COM1, "Failed to open %s\r\n", proc->filename);
        proc->state = TASK_ZOMBIE;
        return NULL;
    }

    if(exec_file->flags != FS_FILE){
        
        close_fs(exec_file);
        kfree(exec_file);
        proc->state = TASK_ZOMBIE;
        return NULL;
    }
    
    //Check whether executable is a suitable elf file
    Elf32_Ehdr elf_header;
    read_fs(exec_file, 0, sizeof(Elf32_Ehdr), (uint8_t*)&elf_header);

    if(memcmp(elf_header.e_ident, "\x7f\x45\x4c\x46", 4) ){
        uart_print(COM1, "Invalid ELF header %s\r\n", proc->filename);
        proc->state = TASK_ZOMBIE;
        return NULL;
    }

    if(elf_header.e_type != ET_EXEC ){
        uart_print(COM1, "Only ELF executables supported %s\r\n", proc->filename);
        proc->state = TASK_ZOMBIE;
        return NULL;
    }


    if(elf_header.e_machine != EM_386 ){
        uart_print(COM1, "Only x86 executables supported %s\r\n", proc->filename);
        proc->state = TASK_ZOMBIE;
        return NULL;
    }


//load program headers

//switch to process address space
    uint32_t oldcr3;
    oldcr3 = get_cr3();
    set_cr3((u32)get_physaddr(proc->page_dir)  );
    flush_tlb();
    
    //inside the process addr space
    uint16_t phdr_count = elf_header.e_phnum;
    Elf32_Phdr *phdr = kmalloc(sizeof(Elf32_Phdr) * phdr_count);
    if(!phdr)
        error("failed to allocate for program headers");
    
    read_fs(exec_file, elf_header.e_phoff, sizeof(Elf32_Phdr) * phdr_count, (uint8_t*)phdr);

    // (Elf32_Phdr*)((unsigned int)elf_header + elf_header.e_phoff);
    for(int i = 0; i < phdr_count; ++i){
        
        if(phdr[i].type == ELF_PT_LOAD){

            //create memory mapping and loading
            //for page alloc memsize!!!! for reading from file file size!!!
            int npages = (phdr[i].memsz /4096) + (phdr[i].memsz % 4096 != 0); //likeceil 
            
            for(int j = 0 ; j < npages; ++j){

                uint8_t* physical_page = allocate_physical_page();
                map_virtaddr(
                                (void *)((phdr[i].vaddr & ~0xffful) + j*0x1000),
                                physical_page,
                                ( (phdr[i].flags & PF_W) ? PAGE_READ_WRITE : 0 ) | PAGE_USER_SUPERVISOR
                                );                    
                
            }

            memset( (void *)(phdr[i].vaddr & ~0xffful), 0, 0x1000 * npages);
            read_fs(exec_file, phdr[i].offset, phdr[i].filesz, (void *)(phdr[i].vaddr));

            struct vma* v = kcalloc(1,sizeof(struct vma));
            v->start = phdr[i].vaddr & ~0xffful;
            v->end = (phdr[i].vaddr & ~0xffful) + (npages * 4096); 
            v->flags = (VM_PRIVATE) | (VM_READ | ((phdr[i].flags & PF_W) ? VM_WRITE : 0));
            v->file_offset = 0;
            v->file = NULL; // only anonymouse pages get freed

            vmm_insert_vma(proc->mm, v);
        }
    }

    kfree(phdr);
    close_fs(exec_file);

    proc->regs.eip = (uint32_t)elf_header.e_entry;

    
    struct vma* vma = kcalloc(1,sizeof(struct vma));
    vma->end = VMM_USERMEM_END;
    vma->start = vma->end - (8 * 0x1000);
    vma->flags = (VM_GROWSDOWN) | (VM_READ | VM_WRITE);
    vma->file = NULL;
    vmm_insert_vma(proc->mm, vma);
    

    //since we are on 32 bit, right? oh wait doesn't matte
    uint32_t * esp = (void *)proc->regs.esp;
           
    esp -= 1;
    *esp = 0;

    esp -= proc->argc;
    char **argv = (char **)esp;
    memset(esp , 0, sizeof(char*) * proc->argc);


    for(int i = 0; i < proc->argc; ++i){

        size_t size = strlen(proc->argv[i]) + 1;
        esp -=  (size / 4) + (size % 4 != 0); //like ceil also make sures that it is aligned?
        memcpy(esp, proc->argv[i], size);
        argv[i] = (char *)esp;
    }

            

    esp = (void *)( (u32)esp & 0xfffffff0ul  );

    // //fuck it we ball
    esp -= 1;
    *esp = (uint32_t)(argv);
    esp -= 1;
    *esp = (uint32_t)proc->argc;

    proc->regs.esp = (u32)esp;
    proc->regs.edi = proc->argc;
    proc->regs.esi = (uint32_t)(argv);


    //switch back the address space
    
    set_cr3(oldcr3);
    flush_tlb();
    _current_process->state = TASK_RUNNING;
    return proc;
}

#include <tty.h>
#include <syscalls.h>
pcb_t * terminate_process(pcb_t * p){

    //tty sessioning
    tty_t* ctty = p->ctty;
    if(ctty){

        if(p->sid == p->pid){ //if process is a session leader

            if(ctty->session == p->pid){
                ctty->session = 0;
                ctty->fg_pgrp = 0;
            }

            //send some sort of signal to the processes?

            ctty = NULL;
        }
    }

    //close open files
    for(int i = 0; i < MAX_OPEN_FILES; ++i){
        close_for_process(p, i);
    }
    
    list_remove(process_list, p->self);
    p->state = TASK_ZOMBIE;

    //also orphan the children and signal the parent as well
    listnode_t * parent = p->parent;
    if(parent){
        
        //move children to the parent process
        listnode_t* lnode;
        pcb_t* pproc = ((pcb_t*)parent->val);
        while( (lnode = list_pop_end(p->childs) ) ){
            pcb_t* child = lnode->val;
            child->parent = pproc->self;
            list_insert_end(pproc->childs, child);
            kfree(lnode);
        }

        
        pid_t ppid = pproc->pid;
        process_send_signal(ppid, SIGCHLD);
        kfree(p->childs);

    }

    if(p->itimer >= 0){
        timer_event_t* e = timer_get_by_index(p->itimer);
        timer_destroy_timer(e);
    }
    return NULL;
}

int context_switch_into_process(struct regs  *r, pcb_t * process){
    

    set_kernel_stack((uint32_t)(void*)process->kstack + 0x1000, &tss_entry);
    set_cr3(process->regs.cr3);

    current_page_dir = (uint32_t *)process->page_dir;
    fpu_load_from_buffer(process->fpu);


    //check if there's a ring change
    //if we -> ring0, we use the saved frame to its stack anyway
    //however ring0 -> ring3 transisitons, we overwrite the interrupt frame
    //which is in the ring0's normal stack
    
    //coming from ring0 to ring3 task then  i must build a new interrupt frame and return from it
    // int ring_transition = 0;
    // if((r->cs & 3) == 0 && (process->regs.cs & 3) == 3){
    //     ring_transition = 3;
        
    // }

    
    //build return frame on the kstack of the process no matter what
    r = (void*)process->kstack;
    // if(ring_transition && ring_transition == 3 || 1){
    //     //build new interrupt frame on ring0 tasks kernel stack
    // }

    r->eax = process->regs.eax;
    r->ebx = process->regs.ebx;
    r->ecx = process->regs.ecx;
    r->edx = process->regs.edx;

    r->edi = process->regs.edi;
    r->esi = process->regs.esi;

    r->ebp = process->regs.ebp;
    r->eip = process->regs.eip;
    r->eflags = process->regs.eflags;
    


    r->cs = process->regs.cs & 0xffff;
    r->ds = process->regs.ds & 0xffff;
    r->es = process->regs.es & 0xffff;
    r->fs = process->regs.fs & 0xffff;
    r->gs = process->regs.gs & 0xffff;
    
    
    //well well well otherwise only works for ring3 tasks
    if( (process->regs.cs & 0x3) == 3){ //ring3 task
        r->ss = process->regs.ss & 0xffff;
        r->useresp = process->regs.esp;
        
    }
    else{
        
        r = (void*)process->regs.esp;    
    }
    

    
    if( _current_process->recv_signals){
        
        //well i should only one bit set so...
        unsigned int sig_num = 0;
        
        for(unsigned int i = _current_process->recv_signals; i > 1 ; i >>= 1){
            sig_num++;
        }
        
        _current_process->recv_signals = 0;


        // uart_print(COM1, "signal number that is : %u\n", sig_num);
        //run the signal handler somehow

        if(_current_process->syscall_state == SYSCALL_STATE_PENDING) {
            _current_process->syscall_state = SYSCALL_STATE_NONE;
            r->eax = -EINTR;
        }
    }

    //now if syscall_state is pending then
    if(_current_process->syscall_state == SYSCALL_STATE_PENDING){

        _current_process->syscall_state = SYSCALL_STATE_NONE;
        syscall_handlers[_current_process->syscall_number](r);
        
        
        //if it still pending then, i hope this won't come around and bite me in the ass :/
        if(_current_process->syscall_state == SYSCALL_STATE_PENDING)
            schedule(r); //schedule again
    }

    
    extern int resume_kthread(struct regs* r);
    return resume_kthread(r);
}

void print_processes(){
    int i = 0;
    for(listnode_t * node = process_list->head; node != NULL; node = node->next){
        pcb_t * p = node->val;
        fb_console_printf("%u -> pid:%u pathname:%s \n", i++, p->pid, p->filename);
        fb_console_printf("\teax:%x \n", p->regs.eax);
        fb_console_printf("\tebx:%x \n", p->regs.ebx);
        fb_console_printf("\tecx:%x \n", p->regs.ecx);
        fb_console_printf("\tedx:%x \n", p->regs.edx);

        fb_console_printf("\teflags:%x \n", p->regs.eflags);

        fb_console_printf("\tedi:%x \n", p->regs.esi);
        fb_console_printf("\tesi:%x \n", p->regs.edi);

        fb_console_printf("\tebp:%x \n", p->regs.ebp);
        fb_console_printf("\tesp:%x \n", p->regs.esp);

        fb_console_printf("\teip:%x \n", p->regs.eip);
    }
    
    
}

void print_current_process(){
    
    
        pcb_t * p = _current_process;
        fb_console_printf("pid:%u pathname:%s \n", p->pid, p->filename);
        fb_console_printf("\teax:%x \n", p->regs.eax);
        fb_console_printf("\tebx:%x \n", p->regs.ebx);
        fb_console_printf("\tecx:%x \n", p->regs.ecx);
        fb_console_printf("\tedx:%x \n", p->regs.edx);

        fb_console_printf("\teflags:%x \n", p->regs.eflags);

        fb_console_printf("\tedi:%x \n", p->regs.esi);
        fb_console_printf("\tesi:%x \n", p->regs.edi);

        fb_console_printf("\tebp:%x \n", p->regs.ebp);
        fb_console_printf("\tesp:%x \n", p->regs.esp);

        fb_console_printf("\teip:%x \n", p->regs.eip);
    
    
    
}


void save_context(struct regs * r, pcb_t * process){
    process->regs.eax = r->eax;
    process->regs.ebx = r->ebx;
    process->regs.ecx = r->ecx;
    process->regs.edx = r->edx;

    process->regs.eflags = r->eflags;

    process->regs.esi = r->esi;
    process->regs.edi = r->edi;

    process->regs.ebp = r->ebp;

    // if((process->regs.cs & 3) == 0 ){ //ring0 task
    if( (r->cs & 3) == 0) {
        process->regs.esp = (uint32_t)r;
    }
    else{ //ring3

        process->regs.esp = r->useresp;
    }


    process->regs.eip = r->eip;
    process->regs.cs = r->cs & 0xffff;
    process->regs.ds = r->ds & 0xffff;
    process->regs.es = r->es & 0xffff;
    process->regs.fs = r->fs & 0xffff;
    process->regs.gs = r->gs & 0xffff;

    //save fpu state as well
    fpu_save_to_buffer(process->fpu);
}


pcb_t * process_get_by_pid(pid_t pid){

    listnode_t * node = process_list->head;
    for(unsigned int  i = 0 ;i < process_list->size; ++i){
        pcb_t * process = node->val;
        if(process->pid == pid){
            return process;
        }
        node = node->next;
    }

    return NULL;

}

//for schedular if no process left return null and cpu will idle
pcb_t * process_get_runnable_process(){

    listnode_t * node = process_list->head;
    for(unsigned int i = 0 ;i < process_list->size; ++i){
        pcb_t * process = node->val;
        if(process->state == TASK_RUNNING){
            return process;
        }
        node = node->next;
    }

    return NULL;
}




void process_release_sources(pcb_t * proc){

    //free the resources
    
    for(int i = 0; i < proc->argc  ; ++i){
        if(proc->argv[i]){

            kfree(proc->argv[i]);
        }
    }
    kfree(proc->argv);

    //switch address spaces temporarily
    uint32_t old_cr3 = get_cr3();
    set_cr3((uint32_t)get_physaddr(proc->page_dir));
    flush_tlb();

    struct mm_struct* mm = proc->mm;
    for(;;){
        struct vma* v = mm->mmap;
        if(!v){   
            break;
        }
        mm->mmap = v->next;

        for(uintptr_t addr = v->start; addr < v->end; addr += 4096){
            
            if(!v->file ){ //ram backed

                if(v->flags & VM_GROWSDOWN){
                    if(!is_virtaddr_mapped((void*)addr)){
                        continue;
                    }
                }
                
                deallocate_physical_page(get_physaddr((void*)addr));
                unmap_virtaddr(((void*)addr));
            }
            else{
                break;
            }
        }
        
        kfree(v);
    }

    kfree(mm);

    

    uint32_t* p_pt_entry = (uint32_t*)proc->page_dir;
    for(int i = 0; i < 768; ++i){
        if(proc->page_dir[i] & 1){ //if present
            //faulty for some cases?
            deallocate_physical_page( (void*) (proc->page_dir[i] & ~0xffful) );
        }
    }
    // kfree(proc->page_dir);
    deallocate_physical_page( get_physaddr(proc->page_dir) );

    deallocate_physical_page( get_physaddr(proc->kstack) );
    kfree(proc->cwd);

    //switch back
    set_cr3(old_cr3);
    flush_tlb();


    process_delete_proc_entry(proc);
}





//exiting 
static void kernel_thread_epilogue(){
    
    //basically syscalls for the threads
    asm volatile(
        "mov $60, %eax\n"
        "mov $0, %ebx\n"
        "int $0x80\n"
    );
}

static int kernel_tid = -1;

pcb_t * create_kernel_process(void (*entry)(void* data), void* data, const char namefmt[], ...){

    pcb_t *task = kcalloc(1, sizeof(pcb_t));
    if(!task){
        error("Failed to allocate pcb_t");
        return NULL;
    }
    // Set up process details
    task->pid = allocate_pid();
    task->state = TASK_RUNNING;
    task->self = list_insert_end(process_list, task);

    va_list va;
    va_start(va, namefmt);
    va_sprintf(task->filename, namefmt, va);
    va_end(va);


    //setting up the page directory
    task->page_dir = kpalloc(1);
    task->regs.cr3 = (uint32_t)get_physaddr(task->page_dir);

    memset(task->page_dir, 0, 4096);
    memcpy(&task->page_dir[768], &kdir_entry[768], (1023 - 768) * 4); //mapping //except last, recursion

    task->page_dir[RECURSIVE_PD_INDEX] = task->regs.cr3;
    task->page_dir[RECURSIVE_PD_INDEX] |= (PAGE_PRESENT | PAGE_READ_WRITE);
        
    
    // Allocate a separate kernel stack for IRQs
    void *kstack = kpalloc(1);
    task->kstack = kstack;
    task->stack_top = (void*)((uint32_t)kstack + 0x1000);

    // Set up registers
    task->regs.eflags = 0x202;
    task->regs.cs = (1 * 8) | 0;
    task->regs.ds = (2 * 8) | 0;
    task->regs.ss = (2 * 8) | 0;
    task->regs.es = (2 * 8) | 0;
    task->regs.fs = (2 * 8) | 0;
    task->regs.gs = (2 * 8) | 0;


    unsigned int* esp = (void*)(task->stack_top);
    esp = (unsigned int*)(uint32_t)esp - sizeof(struct regs);
    struct regs* frame = (void*)esp;
    memset(frame, 0, sizeof(struct regs));

    //in this case the argument and return points are in 
    // ss -> argument
    // useresp return point
    frame->ss = (unsigned int)data;
    frame->useresp = (unsigned int)kernel_thread_epilogue;
    
    frame->gs = 0x10;
    frame->fs = 0x10;
    frame->es = 0x10;
    frame->ds = 0x10;
    frame->esp = (uint32_t)frame;
    frame->eip = (uint32_t)entry;
    frame->cs = 0x8;
    frame->eflags = 0x202;
    

    task->regs.eip = (uint32_t)entry;
    task->regs.esp = (uint32_t)esp;
    task->regs.ebp = 0;

    // **Use kstack for IRQs**
    // set_kernel_stack((uint32_t)kstack + 0x1000, &tss_entry);

    return task;
}


static void idle_task(){

    while(1){
        
        asm volatile("hlt\n\t");
    }
}

pcb_t* create_idle_task(){
    
    pcb_t *task = kcalloc(1, sizeof(pcb_t));

    // Set up process details
    task->pid = 0;
    task->state = TASK_RUNNING;
    task->self = NULL;

    //setting up the page directory
    task->page_dir = kpalloc(1);
    task->regs.cr3 = (uint32_t)get_physaddr(task->page_dir);
    memset(task->page_dir, 0, 4096);
    memcpy(&task->page_dir[768], &kdir_entry[768], (1023 - 768) * 4); //mapping //except last, recursion
    
    //set recursive paging as well
    task->page_dir[RECURSIVE_PD_INDEX] = task->regs.cr3;
    task->page_dir[RECURSIVE_PD_INDEX] |= (PAGE_PRESENT | PAGE_READ_WRITE);

    
    // Allocate a separate kernel stack for IRQs
    void *kstack = kpalloc(1);
    task->kstack = kstack;
    task->stack_top = task->kstack + 0x1000;

    // Set up registers
    task->regs.eflags = 0x206;
    task->regs.cs = (1 * 8) | 0;
    task->regs.ds = (2 * 8) | 0;
    task->regs.ss = (2 * 8) | 0;
    task->regs.es = (2 * 8) | 0;
    task->regs.fs = (2 * 8) | 0;
    task->regs.gs = (2 * 8) | 0;

    unsigned int* esp = (void*)(task->stack_top);
    esp = (unsigned int*)(uint32_t)esp - sizeof(struct regs);
    
    struct regs* frame = (void*)esp;
    memset(frame, 0, sizeof(struct regs));

    frame->gs = 0x10;
    frame->fs = 0x10;
    frame->es = 0x10;
    frame->ds = 0x10;
    frame->esp = (uint32_t)frame;
    frame->eip = (uint32_t)idle_task;
    frame->cs = 0x8;
    frame->eflags = 0x202;
    

    task->regs.esp = (uint32_t)(esp);
    task->regs.ebp = 0;

    task->regs.eip = (uint32_t)idle_task;
    // **Use kstack for IRQs**
    return task;
}



static int _save_current_context_helper (int edi ,int esi,  int ebp,  int old_esp ,
              int ebx,  int edx,  int ecx,  int eax ,int old_ebp,
             int eip ,pcb_t * proc
            )
{
    _current_process->regs.eax = eax;
    _current_process->regs.ebx = ebx;
    _current_process->regs.ecx = ecx;
    _current_process->regs.edx = edx;

    _current_process->regs.esi = esi;
    _current_process->regs.edi = edi;

    _current_process->regs.eip = eip;
    _current_process->regs.ebp = old_ebp;
    _current_process->regs.esp = old_esp;
    _current_process->regs.cs = (1 * 8) | 0;
    _current_process->regs.ds = (2 * 8) | 0;
    _current_process->regs.ss = (2 * 8) | 0;
    _current_process->regs.es = (2 * 8) | 0;
    _current_process->regs.fs = (2 * 8) | 0;
    _current_process->regs.gs = (2 * 8) | 0;

}

void save_current_context(pcb_t * proc){
    
    proc = _current_process;
    asm volatile(
    "pusha\n\t"
    "call _save_current_context_helper\n\t"
    "popa"
    );

    struct context *ctx = &proc->regs;
    ctx->eip = (uint32_t)&&save_label;  // pseudo EIP
save_label:
    return;
}


typedef struct stackframe {
  struct stackframe* ebp;
  uint32_t eip;
} stackframe_t;      

void inkernelstacktrace(){
    uart_print(COM1, "inkernelstacktrace:\r\n");

    struct stackframe* stk;
    asm volatile( "mov %%ebp, %0" : "=r"(stk));

    for(; stk ; stk = stk->ebp){
        uart_print(COM1, "--> %x\r\n",stk->eip );
    }

}


void process_wakeup_list(list_t* wakeuplist){

    //i will assume poping a process from the list
    //effectively clearaing out the list so
    for(listnode_t* node = list_pop_end(wakeuplist);  node ; node = list_pop_end(wakeuplist) ){
        pcb_t* proc = node->val;
        kfree(node);

        //i mean it should be interruptiable or somethin so
        if(proc->state == TASK_INTERRUPTIBLE)
            proc->state = TASK_RUNNING;
    }


}
EXPORT_SYMBOL(process_wakeup_list);

int process_get_empty_fd(pcb_t* proc){

    for(int i = 0; i < MAX_OPEN_FILES; ++i){
        if(!proc->open_descriptors[i].f_inode){
            //temporaly
            proc->open_descriptors[i].f_inode = (void*)1;
            return i; //empty
        }
    }

    // no fd?
    return -1;
}

int process_send_signal(pid_t pid, unsigned int signum){

    if(signum >= 32)
        return -EINVAL;

    if( pid <= 0) return -2; //don't have process gorups so and pid 1


    pcb_t *proc = process_get_by_pid(pid);
    if( !proc ) 
        return -2;

    //if a signal can be sent
    if ( ((proc->state & ( TASK_INTERRUPTIBLE )))  ){
        
        proc->state = TASK_RUNNING;
    }

    
    switch(signum){
        
        case SIGKILL:   __attribute__ ((fallthrough));
        case SIGSEGV:   __attribute__ ((fallthrough));
        case SIGINT:    __attribute__ ((fallthrough));
        case SIGILL:         

        proc->exit_code = signum; //killed by signal
        terminate_process(proc);
        break;
        
        case SIGSTOP:
            

            proc->state = TASK_STOPPED;
            proc->recv_signals = 1 << signum;
            proc->exit_code = signum;
            //notify the parent as well
            pcb_t *p = proc->parent->val;
            if(!p){
                return -1;
            }
            process_wakeup_list(&p->waitqueue);
                
        break;

        case SIGCONT:
            fb_console_printf("sigcont to %u:%s\n", pid, proc->filename);
        __attribute__ ((fallthrough));
        
        default: //do nothing about it
            proc->recv_signals = 1 << signum;
        break;
    }
    
    return 0;
}


int process_send_signal_pgrp(pid_t pgrp, unsigned int signum){

    int err = 0;
    for(listnode_t* node = process_list->head; node; node = node->next){

        pcb_t* proc = node->val;
        if(proc->pgid == pgrp){
            process_send_signal(proc->pid, signum);
            err++;
        }

    }

    return 0;
}   

void copy_context_to_trap_frame(struct regs* r, struct context* ctx){
    r->eax = ctx->eax;
    r->ebx = ctx->ebx;
    r->ecx = ctx->ecx;
    r->edx = ctx->edx;
    r->esi = ctx->esi;
    r->edi = ctx->edi;
    r->ebp = ctx->ebp;

    // stack pointer (only for ring0 tasks)
    r->esp = (unsigned int)r;

    // instruction pointer & flags
    r->eip = ctx->eip;
    r->eflags = ctx->eflags;

    // code segment (kernel CS)
    r->cs = ctx->cs;
    // segment registers must be loaded manually before iret
    r->ds = ctx->ds;
    r->es = ctx->es;
    r->fs = ctx->fs;
    r->gs = ctx->gs;
    return;
}

int _schedule(){
    return kernl_schedule_shim();
}
EXPORT_SYMBOL(_schedule);

file_t* process_get_file(pcb_t* proc, int fd){

    if(fd  < 0 || fd > MAX_OPEN_FILES){
        return NULL;
    }

    if(proc->open_descriptors[fd].f_inode){
        return &proc->open_descriptors[fd];
    }

    return NULL;
}
EXPORT_SYMBOL(process_get_file);

pcb_t* process_get_current_process(){
    return _current_process;
}
EXPORT_SYMBOL(process_get_current_process);
    
    


uint32_t process_proc_cmdline_read(fs_node_t* nnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    struct proc_entry* e = nnode->device;
    pcb_t* proc = e->data;

    size_t totalsize = 0;
    for(int i = 0; i < proc->argc; ++i){
        totalsize += strlen(proc->argv[i]) + 1; //extra null pointer
    }

    if(offset > totalsize){
        return 0;
    }

    size_t tindex = 0;
    char * table = kcalloc(1, totalsize + 8); //extra guard
    for(int i = 0; i < proc->argc; ++i){
        memcpy(&table[tindex], proc->argv[i], strlen(proc->argv[i]) + 1); //copy the null-byte as well
        tindex += strlen(proc->argv[i]) + 1;
    }

    size_t remaining = totalsize - offset;
    if(size > remaining){
        size = remaining;
    }

    memcpy(buffer, &table[offset], size);
    kfree(table);
    return size;
};

uint32_t process_proc_maps_read(fs_node_t* nnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    struct proc_entry* e = nnode->device;
    pcb_t* proc = e->data;

    if(!proc->mm)
        return 0;

    size_t total_s = 0;
    char lbuff[64];
    for(struct vma* v = proc->mm->mmap; v ; v = v->next){
        char* filepath = "";
        if(!v->file){
            if(v->flags & VM_GROWSDOWN){
                filepath = "[stack]";
            }
        }
        else{
            filepath = v->file->name;
        }
        total_s += sprintf(
                            lbuff, "%x-%x r%cx%c %u     %s\n", 
                            v->start,  v->end, 
                            v->flags & VM_WRITE ? 'w' : '-' ,
                            v->flags & VM_SHARED ? 's' : 'p',
                            v->file_offset,
                            filepath
                            );
    }

    if(offset > total_s){
        return 0;
    }

    size_t remain= total_s - offset;
    if(size > remain){
        size = remain;
    }

    char* table = kcalloc(total_s + 16, 1);
    for(struct vma* v = proc->mm->mmap; v ; v = v->next){
        char* filepath = "";
        if(!v->file){
            if(v->flags & VM_GROWSDOWN){
                filepath = "[stack]";
            }
        }
        else{
            filepath = v->file->name;
        }
        sprintf(
                lbuff, "%x-%x r%cx%c %u     %s\n", 
                v->start,  v->end, 
                v->flags & VM_WRITE ? 'w' : '-' ,
                v->flags & VM_SHARED ? 's' : 'p',
                v->file_offset,
                filepath
                );
        strcat(table, lbuff);
    }

    memcpy(buffer, &table[offset], size);
    kfree(table);
    return size;
};
uint32_t process_proc_stat_read(fs_node_t* nnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    struct proc_entry* e = nnode->device;
    pcb_t* proc = e->data;

    int tty_nr;
    tty_t* tty = ((tty_t*)proc->ctty);
    if(!tty){
        tty_nr = 0;
    }
    else{
        int major =  0, minor = 0;
        switch(tty->driver->num){
            case 1: //serial
                major = 4;
                minor = 64; 
                minor += tty->index;
                break;
            case 2: //vt
                major = 4;
                minor += tty->index;
            break;
            case 3: //pts
                major = 136;
                minor = tty->index;
                break;
            default: 
            break;
        }    
        tty_nr = (major << 8) | minor;
    }   
    
    char localbuffer[128];
    char task_state_ch;
    if(proc->state == TASK_RUNNING) task_state_ch = 'R';
    else if(proc->state == TASK_INTERRUPTIBLE) task_state_ch = 'S';
    else if(proc->state == TASK_UNINTERRUPTIBLE) task_state_ch = 'D';
    else if(proc->state == TASK_ZOMBIE) task_state_ch = 'Z';
    else if(proc->state == TASK_STOPPED) task_state_ch = 'T';
        
    size_t len = sprintf(
                            localbuffer, 
                            "%u (%s) %c %u %u %u %u", 
                            proc->pid, proc->filename, 
                            task_state_ch,
                            proc->parent ? ((pcb_t*)(proc->parent->val))->pid : 0,
                            proc->pgid,
                            proc->sid,
                            tty_nr
                        );
    if(offset > len){
        return 0;
    }

    size_t remain = len - offset;
    if(size > remain){
        size = remain;
    }

    memcpy(buffer, &localbuffer[offset], size);
    return size;
};


int process_create_proc_entry(pcb_t* proc){
    char pidname[3] = "000";
    sprintf(pidname, "%u", proc->pid);

    struct proc_entry* pidroot = proc_create_entry(pidname, _IFDIR | 0644, NULL);
    struct proc_entry* cmdline = proc_create_entry("cmdline", _IFREG | 0644, pidroot);
    struct proc_entry* maps = proc_create_entry("maps", _IFREG | 0644, pidroot);
    struct proc_entry* stat = proc_create_entry("stat", _IFREG | 0644, pidroot);

    cmdline->proc_ops.read = &process_proc_cmdline_read;
    maps->proc_ops.read = &process_proc_maps_read;
    stat->proc_ops.read = &process_proc_stat_read;

    pidroot->data = proc;
    cmdline->data = proc;
    maps->data = proc;
    stat->data = proc;

    return 0;
}
int process_delete_proc_entry(pcb_t* proc){
    char entryname[16];

    sprintf(entryname, "%u", proc->pid);
    remove_proc_entry(entryname, NULL);
    //todo remove other entries!!!
    return 0;
}