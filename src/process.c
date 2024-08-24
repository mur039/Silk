#include <process.h>
#include <elf.h>

list_t * process_list;
pcb_t * current_process;
pid_t curr_pid = 0;

void process_init() {
    //is just a double linked list bu its gonna be circular
    process_list = kcalloc(sizeof(list_t), 1);
    process_list->size = 0;
    process_list->head = 0;
    process_list->tail = 0;
    current_process = NULL;

    // register_timer_callback(schedule);
    return;
}

int is_first_time = 1;
void schedule(struct regs * r){

    // print_processes();
    if(process_list->size == 0){
        fb_console_printf("No processes, kernel panics :(\n");
        halt();
    }
    
    
    // print_processes();
    // halt();
    if(current_process == NULL) current_process = process_list->head->val;


    switch(current_process->state){
        case TASK_CREATED: //create the process then bitch
            ;
            pcb_t * p = current_process;
            u8 * code = elf_load(current_process);

            file_t init;    
            init.f_inode = tar_get_file(p->filename, O_RDONLY);
            if(init.f_inode == NULL){
                uart_print(COM1, "Failed to open %s\r\n", p->filename);
                return NULL;
            }
            init.f_mode = O_RDONLY; 

            p->regs.eip = elf_get_entry_address(&init);
            fb_console_printf("Code allocated page:%x\n", code);

            map_virtaddr_d(p->page_dir, p->regs.eip, get_physaddr(code), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);

            void * stack_page = kpalloc(1);//allocate_physical_page();
            map_virtaddr_d(p->page_dir, p->regs.esp, get_physaddr(stack_page), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);



            current_process->state = TASK_RUNNING;
            break;

        case TASK_ZOMBIE:
            current_process = terminate_process(current_process);
            curr_pid = current_process->pid;
            break;

        case TASK_RUNNING:
            //save context?
            if(is_first_time){ is_first_time = 0; return;}
            save_context(r, current_process);
            // print_current_process();
            break;
    }
   

    

    
    if(current_process->self->next){
        current_process = current_process->self->next->val;
    }
    else{
        current_process = process_list->head->val;
    }
    
    if(current_process->state != TASK_RUNNING){
        schedule(r);
    }
    context_switch_into_process(r, current_process);
}



pid_t allocate_pid(){
    return curr_pid++;
}

 pcb_t * create_process(char * filename) {
    // Create and insert a process, the pcb struct is in kernel space
    pcb_t * p1 = kcalloc(sizeof(pcb_t), 1);

    p1->pid = allocate_pid();
    p1->regs.eip = NULL; //schedular will do that shi
    p1->regs.eflags = 0x206; // enable interrupt
    memcpy(p1->filename, filename, strlen(filename));

    p1->self = list_insert_end(process_list, p1);

    // 4kb initial stack
    p1->stack = (void*)0xC0000000;
    p1->regs.esp = (0xC0000000 - 1);
    p1->regs.ebp = 0; //for stack trace

    // Create an address space for the process, how ?
    // kmalloc a page directory for the process, then copy the entire kernel page dirs and tables(the frames don't have to be copied though)
    p1->page_dir = kpalloc(1);//allocate_physical_page();
    //map_virtaddr(p1->page_dir, p1->page_dir, PAGE_PRESENT | PAGE_READ_WRITE);
    memset(p1->page_dir, 0, 4096);

    p1->page_dir[768] = kdir_entry[768]; //mapping the kernel
    p1->page_dir[1012] = kdir_entry[1012]; //and framebuffer?
    
    p1->state = TASK_CREATED;
    return p1;

}

pcb_t * terminate_process(pcb_t * p){
    //i didn't allocated page directory and pagetable with malloc so i can't just deallocate them
    //u32 * directory_entry = p->page_dir;
    // will just lose the link and free p
    pcb_t * ret;
    listnode_t * head = p->self;
    head->prev->next = head->next;
    head->next->prev = head->prev;
    ret = head->next;
    kfree(p);
    process_list->size -= 1;
    return ret;
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
    r->useresp = process->regs.esp;

    r->eflags = process->regs.eflags;
    r->eip = process->regs.eip;
    
    //change pde
    
    asm volatile ( "movl %0, %%cr3"
    : 
    : "a"(get_physaddr(process->page_dir)) 
    :
    );
    flush_tlb();
    return 0;
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
    process->regs.esp = r->useresp;

    process->regs.eip = r->eip;
}