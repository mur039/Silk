#include <process.h>
#include <elf.h>
#include <v86.h>
#include <fb.h>

list_t * process_list;
pid_t curr_pid;
pcb_t * current_process;

static inline pcb_t* next_process(pcb_t * cp){
    pcb_t * ret_val;
    if(!cp->self) return (pcb_t *)process_list->head->val;
    if(cp->self->next){
        ret_val = cp->self->next->val;
    }
    else{
        ret_val = process_list->head->val;
    }
    return ret_val;
}   

void process_init() {
    //is just a double linked list bu its gonna be circular
    process_list = kcalloc(sizeof(list_t), 1);
    process_list->size = 0;
    process_list->head = 0;
    process_list->tail = 0;
    current_process = NULL;
    curr_pid = 0;

    // register_timer_callback(schedule);
    return;
}


int is_first_time = 0;
void schedule(struct regs * r){

    if(process_list->size == 0){
        fb_console_printf("No processes, kernel panics :(\n");
        halt();
    }
        
    if(current_process == NULL) current_process = process_list->head->val;


    switch(current_process->state){
        case TASK_CREATED: //create the process then bitch
            ;
            
            if(current_process->regs.eflags & (1u << V86_VM_BIT)){ //v86 task which its initialization is done uglt but anyway
                current_process->state = TASK_RUNNING;
                break;
            }

            pcb_t * p = current_process;
            u8 * code = elf_load(current_process);

            if(code == NULL){
                p->state = TASK_ZOMBIE;
                break;
            }
            file_t init;    
            init.f_inode = tar_get_file(p->filename, O_RDONLY);
            if(init.f_inode == NULL){
                uart_print(COM1, "Failed to open %s\r\n", p->filename);
                return; //error of sort?
            }

            p->regs.eip = (uint32_t)elf_get_entry_address(&init);
            // fb_console_printf("Code allocated page:%x\n", code);

            int size = 1 + (elf_get_filesz(&init)/4096 + (elf_get_filesz(&init) % 4096 != 0) );
            // fb_console_printf("process size: %u\n", size);

            vmem_map_t * mp;

            

            for(int i = 0; i < size; ++i){
                map_virtaddr_d((void *)p->page_dir, (void *)((p->regs.eip & ~0xffful) + i*0x1000), get_physaddr(code + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
                
                mp = kmalloc(sizeof(vmem_map_t));
                mp->vmem = (p->regs.eip & ~0xffful) + i*0x1000;
                mp->phymem = get_physaddr(code + i*0x1000);
                list_insert_end(p->mem_mapping, mp);

            }
            
            
            void * stack_page = kpalloc(1);//allocate_physical_page();
            map_virtaddr_d((void *)p->page_dir, (void *)p->regs.esp - 0x1000, get_physaddr(stack_page), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);


            mp = kmalloc(sizeof(vmem_map_t));
            mp->vmem = p->regs.esp - 0x1000;
            mp->phymem = get_physaddr(stack_page);
            list_insert_end(p->mem_mapping, mp);
            

            uint32_t oldcr3;
            oldcr3 = set_cr3( (u32)get_physaddr(p->page_dir)  );
            flush_tlb();

            //since we are on 32 bit, right?
            uint32_t * esp = (void *)p->regs.esp;
            
            esp -= p->argc;
            char **argv = (char **)esp;
            memset(esp , 0, 4 * p->argc);


            for(int i = 0; i < p->argc; ++i){
                size_t size = strlen(p->argv[i]) + 1;
                esp -=  (size / 4) + (size % 4 != 0); //like ceil also make sures that it is aligned?
                memcpy(esp, p->argv[i], size);
                argv[i] = (char *)esp;
                // fb_console_printf("argv[%u] = %x : %s\n", i, argv[i], argv[i]);

            }

            
            // argv = (void **)argv[0];

            esp = (void *)( (u32)esp & 0xfffffff0ul  );

            // //fuck it we ball
            esp -= 1;
            *esp = (uint32_t)(argv);
            esp -= 1;
            *esp = (uint32_t)p->argc;

            p->regs.esp = (u32)esp;
            p->regs.edi = p->argc;
            p->regs.esi = (uint32_t)(argv);

            // fb_console_printf("kernel : argc:%u argv:%x\n", p->argc, argv);

            set_cr3(oldcr3);
            flush_tlb();

            current_process->state = TASK_RUNNING;
            break;

        case TASK_ZOMBIE:
            ;
            pcb_t * np = next_process(current_process);
            // curr_pid = current_process->pid;
            terminate_process(current_process);
            current_process = np;
            //print_processes();


            break;

        case TASK_RUNNING:
            //save context?
            if(is_first_time){ is_first_time = 0; return;}
            save_context(r, current_process);
            // print_current_process();
            break;
        
        case TASK_LOADING:
            save_context(r, current_process);
            break;
        
        case TASK_INTERRUPTIBLE:
            fb_console_printf("%u:%s is in task interruptible\n", current_process->pid, current_process->filename);
            save_context(r, current_process);   

            current_process = next_process(current_process);
            
            
            // fb_console_printf("%u:%s in task interruptible next \n", current_process->pid, current_process->filename);
            //may be we also store a mutex or semaphore for other blocking types but not now
            break;

    }
   
    
    current_process = next_process(current_process);

    if( !(current_process->state == TASK_RUNNING || current_process->state == TASK_INTERRUPTIBLE) ){
        schedule(r);
    }


    context_switch_into_process(r, current_process);
}



pid_t allocate_pid(){
    return curr_pid++;
}

 pcb_t * create_process(char * filename, char **_argv) {
    // Create and insert a process, the pcb struct is in kernel space
    pcb_t * p1 = kcalloc(sizeof(pcb_t), 1);

    // //passing the arguments obv
    for(p1->argc = 0 ; _argv[p1->argc] != NULL; p1->argc++);
    p1->argv = kcalloc( p1->argc, sizeof(char *));

    for(int i = 0; i < p1->argc; ++i){
        p1->argv[i] = kmalloc(strlen(_argv[i]) + 1);
        memcpy(p1->argv[i], _argv[i], strlen(_argv[i]) + 1);
    }


    p1->pid = allocate_pid();
    p1->regs.eip = 0; //schedular will do that shi
    p1->regs.eflags = 0x206; // enable interrupt
    memcpy(p1->filename, filename, strlen(filename));

    p1->self = list_insert_end(process_list, p1);

    // 4kb initial stack
    p1->stack = (void*)0xC0000000;
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
    // kmalloc a page directory for the process, then copy the entire kernel page dirs and tables(the frames don't have to be copied though)

    p1->page_dir = kpalloc(1);//allocate_physical_page();
    p1->regs.cr3 = (u32)get_physaddr(p1->page_dir);

    
    //map_virtaddr(p1->page_dir, p1->page_dir, PAGE_PRESENT | PAGE_READ_WRITE);
    memset(p1->page_dir, 0, 4096);
    memcpy(&p1->page_dir[768], &kdir_entry[768], (1024 - 768) * 4); //mapping
    

    
    p1->state = TASK_CREATED;
    


    return p1;

}

pcb_t * terminate_process(pcb_t * p){

    pcb_t * parent = p->parent;
    if(parent){
        parent->regs.eax = p->pid;
        parent->state = TASK_RUNNING; //resume the parent for now
    }

    list_remove(process_list, p->self);
    

    kfree(p);    

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
    r->useresp = process->regs.esp;

    r->eflags = process->regs.eflags;
    r->eip = process->regs.eip;

    r->cs = process->regs.cs;
    r->ds = process->regs.ds;
    r->es = process->regs.es;
    r->ss = process->regs.ss;
    r->fs = process->regs.fs;
    r->gs = process->regs.gs;

    //change pde

    asm volatile ( 
        "movl %0, %%cr3"
    : 
    : "a"( process->regs.cr3) 
    :
    );
    // current_page_dir = process->page_dir;
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

    process->regs.cs = r->cs;
    process->regs.ds = r->ds;
    process->regs.es = r->es;
    process->regs.ss = r->ss;
    process->regs.fs = r->fs;
    process->regs.gs = r->gs;
}