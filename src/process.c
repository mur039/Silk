#include <process.h>
#include <elf.h>
#include <v86.h>

#include <fb.h>
#include <gdt.h>
#include <queue.h>
#include <syscalls.h>
queue_t ready_queue;
queue_t blocked_queue;

list_t * process_list;
pid_t curr_pid ;
pcb_t * current_process;

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
    current_process = NULL;
    curr_pid = 1;

    ready_queue = queue_create(-1); //no limit
    blocked_queue = queue_create(-1); //no limit
    timer_register(10, 10, ( void (*)(void*))schedule, NULL);
    
    task0 = create_idle_task();
    return;
}


int newschedule(struct regs * r){
    fb_console_printf("How the fuck i get here?\n");
    if(!current_process){
        current_process = queue_dequeue_item(&ready_queue);   
    }

    else{ //readl thing
        save_context(r, current_process); //obv
        current_process = queue_dequeue_item(&ready_queue);
        if(!current_process){ //no bitches?
            idle();
        }


    }

    context_switch_into_process(r, current_process);
    return 0;
}

extern void ktty();
void schedule(struct regs * r){
    
    // fb_console_put("Inside the shcedule\n");

    if(process_list->size == 0){
        fb_console_printf("No processes, kernel panics :(\n");
        uart_print(COM1, "No processes, kernel panics :(\r\n");
        // ktty();
        halt();
    }
        
    if(current_process == NULL) 
        current_process = process_list->head->val;


    switch(current_process->state){
        case TASK_CREATED:
            ;
            //hacky but
            if(current_process->regs.eflags & (1 << V86_VM_BIT)){ //v8086 proc
                current_process->state = TASK_RUNNING;
                break;
            }

            load_process(current_process);
            break;

        case TASK_RUNNING:
            save_context(r, current_process);
            break;
        
        case TASK_LOADING:
            break;
        
        case TASK_INTERRUPTIBLE: 
            break;

    }

    // current_process = next_process(current_process);

    pcb_t* candinate = NULL;
    for(unsigned int i = 0; i < process_list->size; ++i){
        current_process = next_process(current_process);
        if(current_process->state == TASK_RUNNING){
            candinate = current_process;
            break;
        }
    
    }

    if(!candinate){

        if(current_process == task0) return;
        current_process = task0;
    }


    context_switch_into_process(r, current_process);

    if(current_process->recv_signals){
        
        //well i should only one bit set so...
        unsigned int sig_num = 0;
        
        for(unsigned int i = current_process->recv_signals; i > 1 ; i >>= 1){
            sig_num++;
        }
        
        current_process->recv_signals = 0;


        uart_print(COM1, "signal number that is : %u\n", sig_num);
        //run the signal handler somehow

        if(current_process->syscall_state == SYSCALL_STATE_PENDING) {
            current_process->syscall_state = SYSCALL_STATE_NONE;
            r->eax = -EINTR;
            return;
        }
    }

    //now if syscall_state is pending then
    if(current_process->syscall_state == SYSCALL_STATE_PENDING){

        current_process->syscall_state = SYSCALL_STATE_NONE;
        syscall_handlers[current_process->syscall_number](r);
        
        
        //if it still pending then, i hope this won't come around and bite me in the ass :/
        if(current_process->syscall_state == SYSCALL_STATE_PENDING)
            schedule(r); //schedule again
    }
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
    p1->stack_bottom = p1->stack_top - 4096;
    p1->regs.esp = (0xC0000000);
    p1->regs.ebp = 0; //for stack trace

    p1->regs.cs = (3 * 8) | 3;
    p1->regs.ds = (4 * 8) | 3;
    p1->regs.es = (4 * 8) | 3;
    p1->regs.fs = (4 * 8) | 3;
    p1->regs.gs = (4 * 8) | 3;
    p1->regs.ss = (4 * 8) | 3;;

    p1->mem_mapping = kcalloc(1, sizeof(list_t));
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
    

    
    vmem_map_t* empty_pages = kcalloc(1, sizeof(vmem_map_t));
    empty_pages->vmem = 0x10000;
    empty_pages->phymem = 0xc0000000;
    empty_pages->attributes = (VMM_ATTR_EMPTY_SECTION << 20) | (786432); //3G -> 786432 pages
    list_insert_end(p1->mem_mapping, empty_pages);


    
    p1->cwd = kmalloc(2);
    if(!p1->cwd)
        error("failed to allocate for cwd");
    memcpy(p1->cwd, "/", 2);

    p1->syscall_state =  SYSCALL_STATE_NONE;
    p1->syscall_number = 0;
    return p1;

}

pcb_t * load_process(pcb_t * proc){


    fs_node_t * exec_file = kopen(proc->filename, O_RDONLY);

    if(!exec_file){
        uart_print(COM1, "Failed to open %s\r\n", proc->filename);
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
    vmem_map_t mp;

    for(int i = 0; i < phdr_count; ++i){
        
        if(phdr[i].type == ELF_PT_LOAD){

            //create memory mapping and loading
            
            //for page alloc memsize!!!! for reading from file file size!!!
            int npages = (phdr[i].memsz /4096) + (phdr[i].memsz % 4096 != 0); //likeceil 
            // fb_console_printf("number of pages allocated for process %s:  %u : %x\n", proc->filename, i,  npages);
            
            
            for(int j = 0 ; j < npages; ++j){

                uint8_t* physical_page = allocate_physical_page();
                map_virtaddr(
                                (void *)((phdr[i].vaddr & ~0xffful) + j*0x1000),
                                physical_page,
                                ( (phdr[i].flags & PF_W) ? PAGE_READ_WRITE : 0 ) | PAGE_USER_SUPERVISOR
                                );


                mp.vmem = (phdr[i].vaddr & ~0xffful) + j*0x1000;
                mp.phymem = (u32)physical_page;
                mp.attributes = (1 << 20);
                vmm_mark_allocated(proc->mem_mapping, mp.vmem, mp.phymem, VMM_ATTR_PHYSICAL_PAGE);

            }

            // vaddr might not be aligned
            memset( (void *)(phdr[i].vaddr & ~0xffful), 0, 0x1000 * npages);
            read_fs(exec_file, phdr[i].offset, phdr[i].filesz, (void *)(phdr[i].vaddr));


        }
    }

    kfree(phdr);

    proc->regs.eip = (uint32_t)elf_header.e_entry;

    void * stack_page = allocate_physical_page();//kpalloc(1);//allocate_physical_page();
    map_virtaddr( (void *)proc->regs.esp - 0x1000, stack_page, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    
    mp.vmem = proc->regs.esp - 0x1000;
    mp.phymem = (u32)stack_page;
    mp.attributes = (1 << 20);
    vmm_mark_allocated(proc->mem_mapping, mp.vmem, mp.phymem, VMM_ATTR_PHYSICAL_PAGE);

    


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
    current_process->state = TASK_RUNNING;
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

    
    return NULL;
}

int context_switch_into_process(struct regs  *r, pcb_t * process)
{
    r->eax = process->regs.eax;
    r->ebx = process->regs.ebx;
    r->ecx = process->regs.ecx;
    r->edx = process->regs.edx;

    r->edi = process->regs.edi;
    r->esi = process->regs.esi;

    r->ebp = process->regs.ebp;
    r->eip = process->regs.eip;
    r->eflags = process->regs.eflags;
    


    r->cs = process->regs.cs;
    r->ds = process->regs.ds;
    r->es = process->regs.es;
    r->fs = process->regs.fs;
    r->gs = process->regs.gs;
    r->ss = process->regs.ss;

    r->useresp = process->regs.esp;

    set_kernel_stack((uint32_t)(void*)process->kstack + 0x1000, &tss_entry);
    asm volatile ( 
        "movl %0, %%cr3"
    : 
    : "a"( process->regs.cr3) 
    :
    );
    
    current_page_dir = (uint32_t *)process->page_dir;

    //well well well otherwise only works for ring3 tasks
    if( (process->regs.cs & 0x3) == 3){ //ring3 task

        //update kernel stack for the process
        return 0;
    }
    else{
        
        r->esp = process->regs.esp;

        return 0;
        struct regs* newstack_r = (void*)r->esp;
        newstack_r -= 1;
        *newstack_r = *r; //copy it over
        r->esp = (uint32_t)newstack_r;

        asm volatile(
            "mov %0, %%esp\n"
            "pop %%gs\n"
            "pop %%fs\n"
            "pop %%es\n"
            "pop %%ds\n"
            "popa\n"
            "add $8, %%esp\n"
            "iret\n"
            :
            : "r"(r->esp)
            : "memory"
        );       
    }
    
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
    
    
        pcb_t * p = current_process;
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

    if((process->regs.cs & 3) == 0){ //ring0 task
        process->regs.esp = r->esp;
    }
    else{ //ring3

        process->regs.esp = r->useresp;
    }


    process->regs.eip = r->eip;

    process->regs.cs = r->cs;
    process->regs.ds = r->ds;
    process->regs.es = r->es;
    process->regs.ss = r->ss;
    process->regs.fs = r->fs;
    process->regs.gs = r->gs;
}


void list_vmem_mapping(pcb_t * process){
    
    listnode_t * node = process->mem_mapping->head;
    for(unsigned int i = 0 ; i < process->mem_mapping->size; ++i){
        vmem_map_t * mmap = node->val;
        u16 vpage_attribute = mmap->attributes >> 20; //upper 12bits

        
        // fb_console_printf("%s -> vmem :%x pmem :%x\n", vpage_attribute ? "ALLOCATED" : "FREE", mmap->vmem, mmap->phymem);
        uart_print(COM1, "%s -> vmem :%x pmem :%x\r\n", vpage_attribute ? "ALLOCATED" : "FREE", mmap->vmem, mmap->phymem);
        
        
        
        node = node->next;
    }
    uart_print(COM1, "-----------------------------------------------------------\r\n\n");     
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

     //free the resources and allocate new ones
    
    for(int i = 0; i < proc->argc  ; ++i){
        if(proc->argv[i]){

            kfree(proc->argv[i]);
        }
    }
    kfree(proc->argv);


    
    for(;;){
        listnode_t* node = list_remove(proc->mem_mapping, proc->mem_mapping->head);
        if(!node) break;


        vmem_map_t * vmap = node->val;

        if(vmap->attributes >> 20){ //allocated ones
            if(vmap->attributes & ~(1 << 20) == VMM_ATTR_PHYSICAL_PAGE){

            //gotta free the allocated physical pages
            deallocate_physical_page((void*)vmap->phymem);
            unmap_virtaddr_d(proc->page_dir, ((void*)vmap->vmem));
            }
            // uart_print(COM1, "vmem: %x->%x :: %x\r\n", vmap->vmem, vmap->phymem, vmap->attributes >> 20);
        }

        kfree(node->val);
        kfree(node);        
    }
    



    kfree(proc->mem_mapping);


    uint32_t* p_pt_entry = (uint32_t*)proc->page_dir;
    for(int i = 0; i < 768; ++i){
        if(proc->page_dir[i] & 1){ //if present

            deallocate_physical_page( (void*) (proc->page_dir[i] & ~0xffful) );
        }
    }
    // kfree(proc->page_dir);
    deallocate_physical_page( get_physaddr(proc->page_dir) );

    deallocate_physical_page( get_physaddr(proc->kstack) );
    
    kfree(proc->cwd);
}





//exiting 
static int kernel_thread_prologue(){
    
    void (*foo)(void) = ( void (*)(void)  )current_process->regs.eax;
    foo();
    for(; ;) idle();

    return 0;
}

static int kernel_tid = -1;

pcb_t * create_kernel_process(void (*entry)(void) ){

    pcb_t *task = kcalloc(1, sizeof(pcb_t));

    // Set up process details
    task->pid = kernel_tid--;
    task->state = TASK_LOADING;
    
    task->self = list_insert_end(process_list, task);



    //setting up the page directory
    task->page_dir = kpalloc(1);
    task->regs.cr3 = (uint32_t)get_physaddr(task->page_dir);
    memset(task->page_dir, 0, 4096);
    memcpy(&task->page_dir[768], &kdir_entry[768], (1023 - 768) * 4); //mapping //except last, recursion
    
    //set recursive paging as well
    task->page_dir[RECURSIVE_PD_INDEX] = task->regs.cr3;
    task->page_dir[RECURSIVE_PD_INDEX] |= (PAGE_PRESENT | PAGE_READ_WRITE);

    task->stack_top = (void*)0xD0001000;

    // Allocate a separate kernel stack for IRQs
    void *kstack = kpalloc(1);
    task->kstack = kstack;


    // Set up registers
    task->regs.eflags = 0x206;
    task->regs.cs = (1 * 8) | 0;
    task->regs.ds = (2 * 8) | 0;
    task->regs.ss = (2 * 8) | 0;
    task->regs.es = (2 * 8) | 0;
    task->regs.fs = (2 * 8) | 0;
    task->regs.gs = (2 * 8) | 0;

    // **Use the mapped function stack**
    task->regs.esp = 0xD0001000;
    task->regs.ebp = 0xD0001000;

    task->regs.eip = (uint32_t)kernel_thread_prologue;
    task->regs.eax = (uint32_t)entry; // Pass function pointer

    // **Use kstack for IRQs**
    // set_kernel_stack((uint32_t)kstack + 0x1000, &tss_entry);

    // Allocate stack for the function
    void *stack = allocate_physical_page();
    map_virtaddr((void*)0xD0000000, stack, PAGE_PRESENT | PAGE_READ_WRITE);
    memset((void*)0xd0000000, 0, 4096);


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

    task->stack_top = (void*)0xD0001000;

    // Allocate a separate kernel stack for IRQs
    void *kstack = kpalloc(1);
    task->kstack = kstack;

    // Set up registers
    task->regs.eflags = 0x206;
    task->regs.cs = (1 * 8) | 0;
    task->regs.ds = (2 * 8) | 0;
    task->regs.ss = (2 * 8) | 0;
    task->regs.es = (2 * 8) | 0;
    task->regs.fs = (2 * 8) | 0;
    task->regs.gs = (2 * 8) | 0;

    // **Use the mapped function stack**
    task->regs.esp = (uint32_t)(task->kstack + 0x1000);
    task->regs.ebp = 0;

    task->regs.eip = (uint32_t)idle_task;
    
    return task;
}



static int _save_current_context_helper (int edi ,int esi,  int ebp,  int old_esp ,
              int ebx,  int edx,  int ecx,  int eax ,int old_ebp,
             int eip ,pcb_t * proc
            )
{
    proc->regs.eax = eax;
    proc->regs.ebx = ebx;
    proc->regs.ecx = ecx;
    proc->regs.edx = edx;

    proc->regs.esi = esi;
    proc->regs.edi = edi;

    proc->regs.eip = eip;
    proc->regs.ebp = old_ebp;
    proc->regs.esp = old_esp;


    fb_console_printf(
        "\teax:%x\n"
        "\tebx:%x\n"
        "\tecx:%x\n"
        "\tedx:%x\n"
        "\tesp:%x\n"
        "\tebp:%x\n"
        "\tesi:%x\n"
        "\tedi:%x\n"
        ,
        eax,
        ebx,
        ecx,
        edx,
        old_esp,
        ebp,
        esi,
        edi
    );
    return 0;
    

}

void save_current_context(pcb_t * proc){
  
   asm volatile(
    "pusha\n\t"
    "call _save_current_context_helper\n\t"
    "popa"
    // "add $0x8, %esp\n\t"
    );


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
        
        case SIGILL:         __attribute__ ((fallthrough));
        case SIGSEGV:         __attribute__ ((fallthrough));
        case SIGINT:         __attribute__ ((fallthrough));
        case SIGKILL:
        if(pid <= 1){

            return -ENOPERM; //ENOPERM
        }

        proc->regs.eax = signum; //killed by signal
        terminate_process(proc);
        break;
        
        case SIGSTOP:
            ;
            //notify the parent as well
            pid_t ppid = ((pcb_t*)proc->parent->val)->pid;
            process_send_signal(ppid, SIGCHLD);
            
            proc->state = TASK_STOPPED;
        break;
        
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