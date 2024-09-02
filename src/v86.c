#include <v86.h>


pcb_t * v86_create_task(const char * filename){
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

    //to load program withing the first 4kB
    
    map_virtaddr(0 , 0, PAGE_PRESENT | PAGE_READ_WRITE);
    flush_tlb();
    memcpy(0x7c00, &file.f_inode[1], o2d(file.f_inode->size));
    unmap_virtaddr(0);
    flush_tlb();

    // void * pe = kpalloc(1);
    // memset(pe, 0, 4096);
    // uint32_t * p32 = p->page_dir;
    // p32[0] = (uint32_t)get_physaddr(pe);
    // p32[0] |= PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR;
    //first 1Mb is within the first table so i allocate one table

    page_directory_entry_t * dir = p->page_dir;
    page_table_entry_t * table;

    table = kpalloc(1);
    memset(table, 0, 4096);

    dir[0].raw = (uint32_t)get_physaddr(table);
    dir[0].present = 1;
    dir[0].read_write = 1;
    dir[0].user_supervisor = 1;

    fb_console_printf("First table is %x\n", dir[0].raw);
    for(int i = 0; i < 205; ++i){
        table[i].raw = i*0x1000 | PAGE_PRESENT | PAGE_USER_SUPERVISOR | PAGE_READ_WRITE; //no usr tho
    }

    // map_virtaddr_d(p->page_dir, 0x0, 0, PAGE_READ_WRITE | PAGE_USER_SUPERVISOR | PAGE_PRESENT); //enteresan
    //1:1 mapping the first 1MB for v86 task
    return p;

}


void v86_monitor(struct regs * r, pcb_t * task){
    uint8_t *ip;
    uint16_t *stack, *ivt;

    ip = r->cs*16 + r->eip;
    ivt = (void *)0;
    stack = r->ss*16 + r->useresp;

    fb_console_printf("V86 task-> cs:ip->%x:%x sp:%x\n", r->cs, r->eip, stack);

    switch (ip[0]) //iopl sensitive instruction emu
    {
    
    
    case V86_INT_N:
        ;
        int interrupt_no = ip[1];
        uart_print(COM1, "An int instruction calling interrupt %u\r\n", interrupt_no);
       
        if(interrupt_no == 3){ //ilgi istiyo abisi
            // terminate_process(current_process);
            // schedule(r);
            r->eax = 0xbeef;
            r->eip += 2;
            break;
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
        r->cs = ivt[ip[1] * 2 + 1];
        r->eip = ivt[ip[1] * 2];
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
        r->eflags = (1 << V86_IF_BIT) | (1 << V86_VM_BIT) | stack[0];
        r->eflags =  (r->eflags & ~(1 << V86_IF_BIT) )  | ((1 << V86_IF_BIT) * (stack[0] & (1 << V86_IF_BIT)) != 0); //hmmmm
        r->useresp = ((r->useresp & 0xffff) + 2) & 0xffff;
        r->eip = (uint16_t) (r->eip + 1);
        break;

    case V86_IRET:
        r->eip = stack[0];
        r->cs = stack[1];
        r->eflags = (1 << V86_IF_BIT) | (1 << V86_VM_BIT) | stack[2];
        r->eflags =  (r->eflags & ~(1 << V86_IF_BIT) )  | ((1 << V86_IF_BIT) * (stack[2] & (1 << V86_IF_BIT)) != 0); //hmmmm
        r->useresp = ((r->useresp & 0xffff) + 6) & 0xffff;
        break;

    case V86_CLI:
        r->eflags &= ~(1 << V86_IF_BIT);
        r->eip += 1;
        break;

    case V86_STI:
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