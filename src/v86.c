#include <v86.h>


pcb_t * v86_create_task(char * filename){
    file_t file;
    file.f_inode = tar_get_file(filename, O_RDONLY);
    if(!file.f_inode){
        return NULL;
    }

    char * argv[] = {filename,  NULL};
    pcb_t * p = create_process( filename, argv);
    //1M and below is not managed by pmm so i'll do manual shit
    p->regs.eflags |= 1ul << V86_VM_BIT;
    p->regs.eip = 0x7c00;
    p->regs.esp = 0xffff;
    p->regs.cs = 0;
    p->regs.ds = 0;
    p->regs.es = 0;
    p->regs.fs = 0;
    p->regs.gs = 0;

    
    //1:1 mapping the first 1MB for v86 task
    map_virtaddr_d(p->page_dir, 0 , 0, PAGE_PRESENT | PAGE_USER_SUPERVISOR);
    for(int i = 1; i < 205; ++i){
        map_virtaddr_d(p->page_dir, (void*)(i*0x1000) , (void*)(i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    }
    
    map_virtaddr(memory_window, (void *)0x7000, PAGE_PRESENT | PAGE_READ_WRITE);
    memcpy((void *)(((uint32_t)memory_window) + 0xc00), &file.f_inode[1], 512); // a sector
    unmap_virtaddr(memory_window);

    //kernel stack 
    void * page = kpalloc(1);
    p->kstack = page;
    memset(page, 0, 4096);

    // p->state = TASK_RUNNING;
    flush_tlb();

    return p;

}





void v86_monitor(struct regs * r, pcb_t * task){
    uint8_t *ip;
    uint16_t *stack, *ivt;

    ip = (uint8_t*)(r->cs*16 + r->eip);
    ivt = (void *)0;
    stack = (uint16_t*)(r->ss*16 + r->useresp);

    fb_console_printf("V86 task-> cs:ip->%x:%x sp:%x\n", r->cs, r->eip, stack);

    switch (ip[0]) //iopl sensitive instruction emu
    {
    
    
    case V86_INT_N:
        ;
        int interrupt_no = ip[1];
        uart_print(COM1, "An int instruction calling interrupt %u\r\n", interrupt_no);
       
       //turn it into a syscall of sort?
        if(interrupt_no == 3){ //ilgi istiyo abisi
            
            switch (r->eax & 0xFF) //value in al
            {
            case 0x0: //exit
                current_process->state = TASK_ZOMBIE;
                schedule(r);
                break;
            
            case 1: //puts //es:di daki string
                ;
                u8 * str = (u8*)(r->es*0 + r->edi & 0xffff);
                uart_print(COM1, "v86 :%s\r\n", str);
                break;
            
            case 2: //write es:di-> src, ebx->length
                ;
                u8 * _str = (u8*)(r->es*0x10 + r->edi & 0xffff);
                uart_print(COM1, "v86 : %s\r\n", _str);
                
                // uart_print(COM1, "\r\n");
                break;
            
            case 3: //hex dump, src-> es:edi length->ebx
                ;
                u8 * hex_head = (u8*)(r->es*0x10 + r->edi);
                u16 len = r->ebx;

                for(int i = 0; i < len; ++i){
                    if(i % 16 == 0){


                        if(i){
                        
                        uart_print(COM1, "|");
                        for(int j = 0; j < 16 ; ++j){
                            u8* rh = hex_head + i - 16;
                            uart_print(COM1, "%c", rh[j] <= 32 ? '.' : rh[j] );
                        }

                        uart_print(COM1, "\r\n");
                        } 


                        uart_print(COM1, "%x: ", hex_head + i);
                    }

                    uart_print(COM1, "%x ", hex_head[i]);
                }
                break;
            default:
                break;
            }
            


            r->eax = 1;
            r->eip += 2;
            break;
        }

        // hacky
        if(task->pid < 0){
            r->es = 0;
            
        }


        stack -= 3;
        r->useresp = ((r->useresp & 0xffff) - 6) & 0xffff;

        stack[0] = (uint16_t) (r->eip + 2);
        stack[1] = r->cs;
        stack[2] = (uint16_t) r->eflags;

        if (r->eflags & (1 << V86_IF_BIT))
            stack[2] |= (1u << V86_IF_BIT);
        else
            stack[2] &= ~(1u << V86_IF_BIT);

        r->eflags &= ~(1 << V86_IF_BIT);
        r->cs  = ivt[ interrupt_no * 2 + 1];
        r->eip = ivt[ interrupt_no * 2];
        break;

    //emulation of out/in is slow af maybe extend the tss to +8192 byte :/
    case V86_OUT_AL_DX: //output byte in al to port dx
        uart_print(COM1, "out al, dx port:%x value:%x\r\n", r->edx & 0xffff, r->eax & 0xff);
        outb(r->edx & 0xffff, r->eax & 0xff);
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_OUT_AX_DX:  //output byte in ax to port dx
        uart_print(COM1, "out ax, dx port:%x value:%x\r\n", r->edx & 0xffff, r->eax & 0xffff);
        outw(r->edx & 0xffff, r->eax & 0xffff);
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_IN_AL_DX: //Input byte from I/O port in DX into AL.
        uart_print(COM1, "in al, dx port:%x value:%x\r\n", r->edx & 0xffff, r->eax & 0xff);
        r->eax = (r->eax & 0xffffff00) | inb(r->edx & 0xffff) ;
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_IN_AX_DX: //Input byte from I/O port in DX into AX.
        uart_print(COM1, "in ax, dx port:%x value:%x\r\n", r->edx & 0xffff, r->eax & 0xffff);
        r->eax = (r->eax & 0xffff0000) | inw(r->edx & 0xffff) ;
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_PUSHF:
        uart_print(COM1, "v86:pushf\r\n");
        r->useresp = ((r->useresp & 0xffff) - 2) & 0xffff;
        stack--;
        stack[0] = (uint16_t) r->eflags;
        if (r->eflags & (1 << V86_IF_BIT)){
            stack[0] |= (1 << V86_IF_BIT);
        } else{
            stack[0] &= ~(1 << V86_IF_BIT);
        }
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_POPF:
        uart_print(COM1, "v86:popf\r\n");
        r->eflags = (1 << V86_IF_BIT) | (1 << V86_VM_BIT) | stack[0];
        r->eflags =  (r->eflags & ~(1 << V86_IF_BIT) )  | ((1 << V86_IF_BIT) * (stack[0] & (1 << V86_IF_BIT)) != 0); //hmmmm
        r->useresp = ((r->useresp & 0xffff) + 2) & 0xffff;
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_IRET:
        uart_print(COM1, "v86:iret\r\n");

        if(task->pid >= 0){

            r->eip = stack[0];
            r->cs = stack[1];
            r->eflags = (1 << V86_IF_BIT) | (1 << V86_VM_BIT) | stack[2];
            r->eflags =  (r->eflags & ~(1 << V86_IF_BIT) )  | ((1 << V86_IF_BIT) * (stack[2] & (1 << V86_IF_BIT)) != 0); //hmmmm
            r->useresp = ((r->useresp & 0xffff) + 6) & 0xffff;
            break;
        }

        /*
            uint32_t * recovery_info = current_process->kstack;
            recovery_info[0] = (uint32_t)ebp;
            recovery_info[1] = (uint32_t)ctx;
            pcb_t * p_head = &recovery_info[2];
            *p_head = *current_process;

        */        
        
        u32 * pk = (u32 *)current_process->kstack;
        
        u32 * old_ebp = (u32 *)pk[0];
        context_t * old_v86ctx = (context_t *)pk[1]; 
        pcb_t * old_context = (pcb_t *)&pk[2]; 


        fb_console_printf("[0]:%x [1]:%x [2]:%x\n", old_ebp, old_v86ctx, old_context);
        old_v86ctx->eax = r->eax;
        old_v86ctx->ebx = r->ebx;
        old_v86ctx->ecx = r->ecx;
        old_v86ctx->edx = r->edx;
        old_v86ctx->esp = r->esp;
        old_v86ctx->ebp = r->ebp;
        old_v86ctx->esi = r->esi;
        old_v86ctx->edi = r->edi;
        old_v86ctx->eflags = r->eflags;
        old_v86ctx->eip = r->eip;


        vbe_info_block_t * t = r->edi;
        fb_console_printf("eax: %x\n", r->eax);
        fb_console_printf("edi: %x\n", r->edi);

        fb_console_put("VBE SIGNATURE: ");
        for(int i = 0; i < 4; ++i)
            fb_console_putchar(t->VbeSignature[i]);    
        fb_console_putchar('\n');



        *current_process = *old_context;


        current_process->regs.ebp = old_ebp[0];  
        current_process->regs.esp = old_ebp[0];

        current_process->regs.eip = old_ebp[1];

        // current_process->regs.eflags &= ~(1ul << V86_VM_BIT);
        // current_process->regs.cs = (1 * 8 ) | 0;
        // current_process->regs.ds = (2 * 8 ) | 0;
        // current_process->regs.es = (2 * 8 ) | 0;
        // current_process->regs.fs = (2 * 8 ) | 0;
        // current_process->regs.gs = (2 * 8 ) | 0;


        current_process->state = TASK_LOADING;
        // terminate_process(current_process);
        context_switch_into_process(r, current_process);
        schedule(r);
        break;

    case V86_CLI:
        uart_print(COM1, "v86:cli\r\n");
        r->eflags &= ~(1 << V86_IF_BIT);
        r->eip += 1;
        break;

    case V86_STI:
        uart_print(COM1, "v86:sti\r\n");
        r->eflags = (1 << V86_IF_BIT);
        r->eip += 1;
        break;
    default:
    
        uart_print(COM1, "**************************************\r\n");
        uart_print(COM1, "Byte caused GPF in V86 task :%x\r\n", ip[0]);
        uart_print(COM1, "**************************************\r\n");
        break;
    }
}


int v86_int(int number, context_t* ctx){ //?????? how nigga how??



    //well ebp can be snatch from here and put inside the tss?
    //ok turn k_thread into a v8086 process
    save_current_context(current_process);
    uint32_t* ebp;
    asm volatile("mov %%ebp, %0": "r="(ebp) ::);
    fb_console_printf(" ebp :%x ebp[0]:%x eip[1]:%x\n", ebp, ebp[0], ebp[1]);
    

    /*
    since there's no priviliege switching TSS doesn't switch stacks in interrupts
    we store some information into the k_stack structure and use it to return 
    */

    uint32_t * recovery_info = current_process->kstack;
    recovery_info[0] = (uint32_t)ebp;
    recovery_info[1] = (uint32_t)ctx;
    pcb_t * p_head = &recovery_info[2];
    *p_head = *current_process;

    
    
    

    

    

    //1:1 mapping the first 1MB for v86 task
    map_virtaddr_d(current_process->page_dir, 0 , 0, PAGE_PRESENT | PAGE_USER_SUPERVISOR);
    
    for(u32 addr = 0x1000; addr < 0x100000; addr+= 0x1000){
        if(addr == 0x8000 ){
            continue;

        }

        map_virtaddr_d(current_process->page_dir, (void *)(addr) , (void *)(addr), PAGE_PRESENT | PAGE_USER_SUPERVISOR);
    }

    // for(int i = 1; i < 205; ++i){
    //     if ( i == 8) continue;
    //     map_virtaddr_d(current_process->page_dir, (void *)(i*0x1000) , (void *)(i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    // }
    

    current_process->regs = *ctx;
    current_process->regs.cs = 0;
    current_process->regs.ds = 0;
    current_process->regs.esp = 0xfffc;
    current_process->regs.ebp = 0xfffc;
    current_process->regs.eflags |= (1ul << V86_VM_BIT) | (1ul << V86_IF_BIT);
    // current_process->regs.eflags |= 1ul << 8; //trap flag

    // current_process->kstack = (uint8_t* )ebp;

    current_process->regs.cs = 0;
    current_process->regs.eip = 0x7c00;
    
    uint8_t * phead = 0x7c00;
    phead[0] = 0xcd;
    phead[1] = number;
    

    // u16 * ivt = 0;
    // current_process->regs.eip = ivt[ number * 2];
    // current_process->regs.cs = ivt[ number * 2 + 1];

    current_process->state = TASK_LOADING;
    while(1);


    return 0;
}
