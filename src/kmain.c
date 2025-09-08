#include <multiboot.h>
#include <stdint-gcc.h>
#include <sys.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
#include <acpi.h>   
#include <pmm.h>
#include <str.h>
#include <pit.h>
#include <gdt.h>

#include <timer.h>
#include <dev.h>
#include <ps2.h>
#include <cmos.h>
#include <ps2_mouse.h>
#include <kb.h>
#include <pci.h>
#include <virtio.h>
#include <tty.h>

#include <v86.h>
#include <glyph.h>
#include <fb.h>
#include <bosch_vga.h>
#include <filesystems/tar.h>
#include <filesystems/vfs.h>
#include <filesystems/nulldev.h>
#include <filesystems/proc.h>
#include <filesystems/tmpfs.h>
#include <filesystems/ata.h>
#include <filesystems/fat.h>
#include <filesystems/ext2.h>
#include <filesystems/fs.h>



#include <char.h>
#include <syscalls.h>
#include <elf.h>
#include <process.h>
#include <circular_buffer.h>
#include <pipe.h>
#include <semaphore.h>
#include <queue.h>
#include <socket.h>
#include <network/e1000.h>
#include <network/netif.h>
#include <vt.h>
#include <fpu.h>
#include <module.h>
#include <loop.h>

#include <sound/ac97.h>

extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t kernel_phy_end;
extern uint32_t kernel_phy_start;


extern void jump_usermode();

static inline void reboot_by_ps2(){ //there's no way i can return
    ps2_send_command(0xD1); //write next byte to output port of the controller
    ps2_send_data(0xFE);
}





uint32_t kernel_stack[2048];
uint32_t kernel_syscallstack[1024];


static void in_kernel_yield(struct regs* r){
    save_context(r, current_process);
    schedule(r);
    return;
}


extern void special_syscall_stub();
void special_syscall(struct regs* r){
    
    int request = r->ebx;
    fb_console_printf("Special syscall: request:%u\n", request);

    if(request == 0){
        disable_interrupts();
        r->eax = 1;
        r->eflags &= ~0x200;
        save_context(r, current_process);

        return;
    }
    

    r->eax = -1;    
    return;
}


void dump_stuck_frame(uint32_t* stack){
    for(int i = 0; i < 6; ++i){
        fb_console_printf("%u : %x\n", i, stack[i]);
    }
    return;
}

void ksoftirq_n(void* data){
    
    
    while(1){
        current_process->state = TASK_INTERRUPTIBLE;
        _schedule();
        fb_console_printf("after the shim\n");
    }
    
}

int vkprintf(const char* fmt, va_list va){
    fb_console_va_printf(fmt,va);
    return 0;
}

int kprintf(const char* fmt, ...){
    va_list va;
    va_start(va, fmt);
    vkprintf(fmt, va);
    va_end(va);
    return 0;
}


EXPORT_SYMBOL(kprintf);
EXPORT_SYMBOL(vkprintf);


void kmain(multiboot_info_t* mbd){ //high kernel

    init_uart_port(COM1); //COM1

    //since i initialize the console before malloc, i need to be careful about not printing escape sequences
    uart_print(COM1, "Well mode of the lfb is %u as well as width and height is %u:%u at address %x\n", mbd->framebuffer_type, mbd->framebuffer_width, mbd->framebuffer_height, mbd->framebuffer_addr_lower);
    init_framebuffer((void*)mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height, mbd->framebuffer_type);

    parse_psf(ter_u16n_psf, ter_u16n_psf_len);
    
    uint32_t console_size =  init_fb_console(-1, -1); //maximum size //requires fonts tho
    uint32_t row,col;
    row = console_size & 0xffff;
    col = console_size >> 16;

    fb_console_printf("init console dimensions: %ux%u\n", col, row);
    uart_print(COM1, "init console %ux%u\r\n", col, row );

    fb_console_printf("Framebuffer addr:%x widthxheight: %ux%u bpp:%u pitch:%u\n", mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height, mbd->framebuffer_bpp, mbd->framebuffer_pitch);

    fb_console_printf("Initializing Pyhsical Memory Manager\r\n");
    uart_print(COM1, "Initializing Pyhsical Memory Manager\r\n");
    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(struct multiboot_mmap_entry)){

        // mmt->type; //look for 1;
        multiboot_memory_map_t * mmt = (multiboot_memory_map_t *)(mbd->mmap_addr +i);
        
        // sprintf
        uart_print(COM1, "%x...%x : Size:%x -> Type : %s\r\n", 
        mmt->addr_low,
        mmt->addr_low + mmt->len_low,
        mmt->len_low,
        mmt->type == 1?
        "Available" :
        "Reserved"
        );

        fb_console_printf("%x...%x : Size:%x -> Type : %s\r\n", 
            mmt->addr_low,
            mmt->addr_low + mmt->len_low,
            mmt->len_low,
            mmt->type == 1?
            "Available" :
            "Reserved"
            );

        if(mmt->type == 1 && mmt->addr_low == 0x100000){ //if available push to anc create bitmap
            pmm_init(mmt->addr_low, mmt->len_low);
            uart_print(COM1, "Initialized pmm_init\r\n");
            fb_console_printf("Initialized pmm_init\r\n");
        }
    }

    uart_print(COM1, "Modules found : %x, At : %x\r\n", mbd->mods_count, mbd->mods_addr);
    fb_console_printf("Modules found : %x, At : %x\r\n", mbd->mods_count, mbd->mods_addr);

    multiboot_module_t * modules  = (multiboot_module_t *)mbd->mods_addr;
    for(unsigned int i = 0; i < mbd->mods_count; i++){
        uart_print(COM1, 
        "Module %x:\r\n"
        "\tcmdline: %s\r\n"
        "\tstart address: 0x%x\r\n"
        "\tend address  : 0x%x\r\n"
        "\tmodule size  : %x\r\n" ,
        i,
        (char*)modules[i].cmdline,
        modules[i].mod_start,
        modules[i].mod_end,
        modules[i].mod_end - modules[i].mod_start
        );

        fb_console_printf(
            "Module %x:\r\n"
            "\tcmdline: %s\r\n"
            "\tstart address: 0x%x\r\n"
            "\tend address  : 0x%x\r\n"
            "\tmodule size  : %x\r\n" ,
            i,
            (char*)modules[i].cmdline,
            modules[i].mod_start,
            modules[i].mod_end,
            modules[i].mod_end - modules[i].mod_start
            );
    }

    uart_print(COM1, "Installing Global Descriptor Tables\r\n");
    gdt_install();
    
    uart_print(COM1, "Installing Interrupt Descriptor Tables\r\n");
    idt_install();

    uart_print(COM1, "Installing ISRs\r\n");
    isrs_install();

    irq_install();
    irq_install_handler(PS2_KEYBOARD_IRQ, keyboard_handler);
    irq_install_handler(        UART_IRQ, uart_handler);
    // irq_install_handler(ATA_MASTER_IRQ, ata_master_irq_handler);

    timer_install();
    timer_phase(TIMER_FREQUENCY); // a ms timer
    
    idt_set_gate(0x81, (unsigned)special_syscall_stub, 0x08, 0xEE); // 0x8E); //special syscall
    

    //fiz paging problems
    virt_address_t addr;
    addr.address = (uint32_t)align2_4kB(kernel_heap);
    addr.table += 1;
    for( ; addr.table < 1023; addr.table++){
        unmap_virtaddr((void *)addr.address);
    } 

    
    addr.address = (uint32_t)get_physaddr(kernel_heap);
    for(unsigned int i = 0; i < 1 + (addr.address - 0x100000)/4096 ; ++i){
        pmm_mark_allocated((void *)0x100000 + 0x1000*i);
    }

    extern uint32_t kernel_start;
    extern uint32_t kernel_data_start;
    
    //map kernel text and rodata as well readonly
    for(uint8_t* start = (uint8_t*)((uint32_t)(&kernel_start) & ~0xFFFul); start < (uint8_t*)&kernel_data_start; start += 4096 ){
        set_virtaddr_flags(start, PAGE_PRESENT);
    }


    for(size_t j = 0; j < mbd->mods_count; ++j){
        
        for(unsigned int i = 0; i < (modules[j].mod_end - modules[j].mod_start)/4096 + ((modules[j].mod_end - modules[j].mod_start) % 4096 != 0) ; ++i){
            
            pmm_mark_allocated((void *)modules[j].mod_start + i*0x1000);
        }
    }



    //while there also allocate pde from 768-1023 so that if kernel map changes, it will be present in all processes
    for(int i =  768; i < 1024; ++i){ //last entry reserved for recursive paging
        //
        if(!kdir_entry[i]){ //it is empty then
            uint8_t* physical_page = allocate_physical_page();
            kdir_entry[i] = (uint32_t)physical_page;
            kdir_entry[i] |= PAGE_PRESENT | PAGE_READ_WRITE;
            
            uint32_t* freetable = ((uint32_t *)0xFFC00000) + (0x400 * i);
            memset(freetable, 0, 4096);
        }
    }
    

    kdir_entry [mbd->framebuffer_addr_lower >> 22 ] |= PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLED;
    for(size_t start = 0; start <= (mbd->framebuffer_width*mbd->framebuffer_height*4); start+= 0x1000 ){
        set_virtaddr_flags((void*)(mbd->framebuffer_addr_lower + start), PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLED | PAGE_READ_WRITE | PAGE_PRESENT);
    }   
    

    flush_tlb();


    //initialize the kernel allocator
    pmm_print_usage();
    kmalloc_init(300); // a MB at front reduces overhead
    alloc_print_list();
    
    fb_console_printf("Initializing Device Manager...\n");
    dev_init();
    
    fb_console_printf("Probing ATA Devices...\n");
    ata_find_devices();
    
    vfs_install();  
    
    
    //well gotta do something like check if we have an initrd right?
    if(mbd->mods_count ){ //there are modules but
        fb_console_printf("Modules found gonna use first module as ustar initrd\n");
        
        //turn it into to driver of sort? //maybe not
        size_t tar_size = (modules[0].mod_end - modules[0].mod_start);
        size_t tar_page_count = (tar_size / 4096) + (tar_size % 4096 != 0); //like ceil
    
        void * tar_begin = vmm_get_empty_kernel_page(tar_page_count, (void*)0xD0000000);
    
        for(unsigned int i = 0; i < tar_page_count ; ++i){
            map_virtaddr( (void*)((uint32_t)tar_begin + i*0x1000), (void *)(modules->mod_start + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE);
        }    
        tar_add_source(tar_begin);
    
        fs_node_t* ramfs = tmpfs_install();
        fs_node_t* tar_node = tar_node_create(tar_begin, tar_size);
        vfs_copy(tar_node, ramfs, 0);
        
        //after unpacking tar into the tmpfs, deallocate the space used by the tar
        for(unsigned int i = 0; i < tar_page_count ; ++i){
            
            uint8_t* addr = (uint8_t*)tar_begin;
            kpfree(addr + i*0x1000);
        }    
    
        vfs_mount("/", ramfs);
        
    }
    else{

        //if no module is found then
        fb_console_printf("No initrd provided looking for root device in cli arguments...\n");
        const char* cmd = (const char*)mbd->cmdline;
        int index = -1;
        for(size_t i = 0; i < strlen(cmd) - strlen("root="); ++i ){
            if(!strncmp(&cmd[i], "root=", 5)){
                index = i;
                break;
            }
        }

        if(index == -1){
            fb_console_printf("No root= argument found. Halting...\n");
            halt();
        }


        char* root_path = NULL;
        const char * _s = &cmd[index + 5];
        for(int i = 0; _s[i] != '\0' ;++i ){

            if(_s[i] == ' '){
                root_path = kcalloc(i + 1, 1);
                memcpy(root_path, _s, i);
                break;
            }
        }

        fb_console_printf("root= index is :%s\n", root_path);
        
        //get the last part
        const char* devname = root_path;
        for(int i = strlen(devname); i >= 0; i--){
            if(_s[i] == '/'){
                devname += i + 1;
                break;
            }
        }

        fb_console_printf("Device name is = %s\n", devname);
        device_t* dev = dev_get_by_name(devname);
        if(!dev){
            fb_console_printf("Failed to find device: %s\n", devname);
            halt();
        }
        
        kfree(root_path);
        struct fs_node * fnode = kcalloc(1, sizeof(struct fs_node));
        fnode->inode = 1;
		
        strcpy(fnode->name, dev->name);
        fnode->uid = 0;
	    fnode->gid = 0;
		fnode->device = dev;

        fnode->flags   = dev->dev_type == DEVICE_CHAR?  FS_CHARDEVICE  : FS_BLOCKDEVICE;
        fnode->ops = dev->ops;
        vfs_mount("/", ext2_node_create(fnode));
        // halt();
    }
    



    vfs_mount("/dev", devfs_create());
    vfs_mount("/proc", proc_create());

    uart_print(COM1, "Framebuffer at %x : %u %u %ux%u\r\n", 
                mbd->framebuffer_addr_lower,
                mbd->framebuffer_bpp,
                mbd->framebuffer_pitch,
                mbd->framebuffer_width,
                mbd->framebuffer_height
                 );

    
    pmm_print_usage();    

    

    // rsdp_t rsdp = find_rsdt();  
    // if(!rsdp_is_valid(rsdp)){
    //     fb_console_put("Failed to find rsdp in the memory, halting...");
    //     uart_print(COM1, "Failed to find rsdp in the memory, halting...");
    //     halt();
    // }


    // RSDT_t * rsdt = (void *)rsdp.RsdtAddress;
    // if(!is_virtaddr_mapped(rsdt) ){
    //     map_virtaddr(rsdt, rsdt, PAGE_PRESENT | PAGE_READ_WRITE);
    // }
    
    // if(!doSDTChecksum(&rsdt->h)){
    //     fb_console_printf("Invalid rsdt :(\n");
    //     return;
    // }
    
    // fb_console_printf("other sdts:\n");
    // int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    // ACPISDTHeader_t **h = (ACPISDTHeader_t**)&rsdt->PointerToOtherSDT; //it's a table u idiot
    // for(int i = 0; i < entries; ++i){
        
    //     char signature[5] = {0};
    //     memcpy(signature, h[i]->Signature, 4);
    //     fb_console_printf("->%u : %s\n", i, signature);
    // }


    // ACPISDTHeader_t* madt = find_sdt_by_signature(rsdt, "APIC");
    // if(madt){
    //     int result = acpi_parse_madt(madt);
    //     if(result != 0){
    //         fb_console_printf("Failed to parse madt table\n");
    //         halt();
    //     }
    // }
    

   
// like high kevel thingy

    initialize_syscalls();
    install_syscall_handler(    SYSCALL_OPEN, syscall_open);
    install_syscall_handler(    SYSCALL_EXIT, syscall_exit);
    install_syscall_handler(    SYSCALL_READ, syscall_read);
    install_syscall_handler(   SYSCALL_WRITE, syscall_write);
    install_syscall_handler(   SYSCALL_CLOSE, syscall_close);
    install_syscall_handler(   SYSCALL_LSEEK, syscall_lseek);
    install_syscall_handler(  SYSCALL_EXECVE, syscall_execve);
    install_syscall_handler(   SYSCALL_IOCTL, syscall_ioctl);
    install_syscall_handler(   SYSCALL_FSTAT, syscall_fstat);
    install_syscall_handler(    SYSCALL_FORK, syscall_fork);
    install_syscall_handler(    SYSCALL_DUP2, syscall_dup2);
    install_syscall_handler(  SYSCALL_GETPID, syscall_getpid);
    install_syscall_handler(    SYSCALL_PIPE, syscall_pipe);
    install_syscall_handler(   SYSCALL_WAIT4, syscall_wait4);
    install_syscall_handler(    SYSCALL_MMAP, syscall_mmap);
    install_syscall_handler(    SYSCALL_KILL, syscall_kill);
    install_syscall_handler(SYSCALL_GETDENTS, syscall_getdents);
    install_syscall_handler(  SYSCALL_GETCWD, syscall_getcwd);
    install_syscall_handler(   SYSCALL_CHDIR, syscall_chdir);
    install_syscall_handler( SYSCALL_SYSINFO, syscall_sysinfo);
    install_syscall_handler(   SYSCALL_MOUNT, syscall_mount);
    install_syscall_handler(  SYSCALL_UNLINK, syscall_unlink);
    install_syscall_handler(   SYSCALL_MKDIR, syscall_mkdir);
    install_syscall_handler(   SYSCALL_PIVOT_ROOT, syscall_pivot_root);
    install_syscall_handler(   SYSCALL_FCNTL, syscall_fcntl);
    install_syscall_handler(   SYSCALL_PAUSE, syscall_pause);
    install_syscall_handler( SYSCALL_GETPPID, syscall_getppid);
    install_syscall_handler( SYSCALL_GETPGRP , syscall_getpgrp);
    install_syscall_handler(SYSCALL_SETPGID , syscall_setpgid);
    install_syscall_handler(SYSCALL_SETSID , syscall_setsid);
    install_syscall_handler(SYSCALL_NANOSLEEP , syscall_nanosleep);
    install_syscall_handler(SYSCALL_SCHED_YIELD , syscall_sched_yield);
    install_syscall_handler(SYSCALL_POLL, syscall_poll);

    initialize_sockets(32);
    initialize_netif();
    module_init();
    module_call_all_initializers();

    enumerate_pci_devices();    
    fb_console_printf("Found PCI devices: %u\n", pci_devices.size);
    for(listnode_t * node = pci_devices.head; node != NULL ;node = node->next ){
        pci_device_t *dev = node->val;

        fb_console_printf("pci-> %u:%u:%u class_code:%x interrupt_line:%u interrupt_pin:%u\n",dev->bus, dev->slot, dev->func, dev->header.common_header.class_code ,dev->header.type_0.interrupt_line, dev->header.type_0.interrupt_pin);        
        // pci_list_capabilities(dev);

        switch(dev->header.common_header.class_code){

            case PCI_NETWORK:
            if( dev->header.common_header.vendor_id == 0x8086 && dev->header.common_header.device_id == 0x100e ){
                initialize_e1000(dev);
            }
            break;

            case PCI_UNCLASSIFIED: break;

            case PCI_MASS_STORAGE:
            break;

            case PCI_DISPLAY:
            break;

            case PCI_MULTIMEDIA:
            if( dev->header.common_header.vendor_id == AC97_PCI_VENDOR_ID && dev->header.common_header.device_id == AC97_PCI_DEVICE_ID ){
                ac97_pci_initialize(dev);
            }
            break;

            case PCI_BRIDGE:
            break;

            // case PCI_MEMORY:
            // break;
            // case PCI_SIMPLE_COMM:
            // break;
            // case PCI_BASE_SYSTEM:
            // break;
            // case PCI_INPUT_DEVICE:
            // break;
            // case PCI_DOCKING:
            // break;
            // case PCI_PROCESSOR:
            // break;
            // case PCI_SERIAL_BUS:
            // break;
            // case PCI_WIRELESS:
            // break;
            // case PCI_INTELLIGENT:
            // break;
            // case PCI_SATELLITE:
            // break;

            default:
            break;

        }


        // if( dev->header.common_header.vendor_id == 0x1af4){
        //     // virtio_register_device(dev);
        // }   
        
        
    }
    
    ps2_kbd_initialize();
    ps2_mouse_initialize();

    create_uart_device(COM1);
    install_virtual_terminals(10, row, col);
    install_kernel_mem_devices();
    
    install_basic_framebuffer((void*)0xfd000000, mbd->framebuffer_width, mbd->framebuffer_height, mbd->framebuffer_bpp);
    loop_install();

    initialize_fpu();
    process_init();
    pcb_t * init =  create_process("/bin/init", (char* []){"/bin/init", NULL} );
    init->sid = 1;
    init->pgid = 1;
    
    create_kernel_process(ksoftirq_n, NULL, "ksoftirq/%u", 0);
    
    jump_usermode();
    //we shouldn't get there //if we do.. well that's imperessive
    for(;;){   
    }

}
