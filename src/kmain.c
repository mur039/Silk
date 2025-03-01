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
#include <filesystems/pex.h>


#include <char.h>
#include <syscalls.h>
#include <elf.h>
#include <process.h>
#include <circular_buffer.h>
#include <pipe.h>
#include <semaphore.h>
#include <queue.h>

extern uint32_t bootstrap_pde[1024];
extern uint32_t bootstrap_pte1[1024];
extern uint32_t kernel_phy_end;
extern uint32_t kernel_phy_start;


extern void jump_usermode();

static inline void reboot_by_ps2(){ //there's no way i can return
    ps2_send_command(0xD1); //write next byte to output port of the controller
    ps2_send_data(0xFE);
}



volatile int  is_received_char = 0;
char ch;


uint32_t kernel_stack[2048];
uint32_t kernel_syscallstack[256];


static void in_kernel_yield(struct regs* r){
    save_context(r, current_process);
    schedule(r);
    return;
}

void kerneltask1(void){

    while(1){
        uart_print(COM1, "Well hello from the kernel task1\r\n");
    }

}


void kmain(multiboot_info_t* mbd){ //high kernel

    init_uart_port(COM1); //COM1
    

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

        if(mmt->type == 1 && mmt->addr_low == 0x100000){ //if available push to anc create bitmap
            pmm_init(mmt->addr_low, mmt->len_low);
            uart_print(COM1, "Initialized pmm_init\r\n");
        }
    }

    uart_print(COM1, "Modules found : %x, At : %x\r\n", mbd->mods_count, mbd->mods_addr);
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
    }

    uart_print(COM1, "Installing Global Descriptor Tables\r\n");
    gdt_install();
    
    set_kernel_stack((uint32_t)&kernel_syscallstack[127], &tss_entry);

    uart_print(COM1, "Installing Interrupt Descriptor Tables\r\n");
    idt_install();

    uart_print(COM1, "Installing ISRs\r\n");
    isrs_install();

    uart_print(COM1, "Installing IRQs\r\n");

    irq_install();
    irq_install_handler(PS2_KEYBOARD_IRQ, keyboard_handler);
    irq_install_handler(        UART_IRQ, uart_handler);
    irq_install_handler(   PS2_MOUSE_IRQ, ps2_mouse_handler);
    // irq_install_handler(ATA_MASTER_IRQ, ata_master_irq_handler);


    //recursive paging
    kdir_entry[RECURSIVE_PD_INDEX] = ((uint32_t)kdir_entry - 0xc0000000) | PAGE_PRESENT | PAGE_READ_WRITE;
    flush_tlb();

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

    for(unsigned int i = 0; i < (modules[0].mod_end - modules[0].mod_start)/4096 + ((modules[0].mod_end - modules[0].mod_start) % 4096 != 0) ; ++i){
        
        pmm_mark_allocated((void *)modules[0].mod_start + i*0x1000);
    }


    
    
    //while there also allocate pde from 768-1023 so that if kernel map changes, it will be present in all processes
    for(int i =  768; i < 1023; ++i){ //last entry reserved for recursive paging
        //
        if(!kdir_entry[i]){ //it is empty then
            uint8_t* physical_page = allocate_physical_page();
            kdir_entry[i] = physical_page;
            kdir_entry[i] |= PAGE_PRESENT | PAGE_READ_WRITE;
            
            //well as we add it to the table, this new page is accesible by
            // ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
            
            uint32_t* freetable = ((uint32_t *)0xFFC00000) + (0x400 * i);
            memset(freetable, 0, 4096);
        }
    }

    //well some motherfucker in 768:1023 doing some shit that when corrected reboots the damn thing
    addr.directory = 768;
    addr.table = 1023;
    addr.offset = 0;
    deallocate_physical_page(get_physaddr(addr.address));
    unmap_virtaddr(addr.address);
    
    

    flush_tlb();

    extern uint32_t kernel_start;
    extern uint32_t kernel_data_start;
    
    //map kernel text and rodata as well readonly
    for(uint8_t* start = (uint8_t*)((uint32_t)(&kernel_start) & ~0xFFFul); start < (uint8_t*)&kernel_data_start; start += 4096 ){
        set_virtaddr_flags(start, PAGE_PRESENT);
    }

    //initialize the kernel allocator
    pmm_print_usage();
    kmalloc_init(255); // a MB at front
    alloc_print_list();
    
    
    


    //turn it into to driver of sort? //maybe not
    size_t tar_size = (modules[0].mod_end - modules[0].mod_start);
    size_t tar_page_count = (tar_size / 4096) + (tar_size % 4096 != 0); //like ceil

    void * tar_begin = vmm_get_empty_kernel_page(tar_page_count, (void*)0xD0000000);

    for(unsigned int i = 0; i < tar_size/4096 + (tar_size % 4096 == 0) ; ++i){
        map_virtaddr( (void*)((uint32_t)tar_begin + i*0x1000), (void *)(modules->mod_start + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE);
        // kernel_heap += 0x1000;
    }    
    tar_add_source(tar_begin);

    



    vfs_install();
  
    
    fs_node_t* ramfs = tmpfs_install();
    fs_node_t* tar_node = tar_node_create(tar_begin, tar_size);
    vfs_copy(tar_node, ramfs, 0);

    //after unpacking tar into the tmpfs, deallocate the space used by the tar
    for(unsigned int i = 0; i < tar_page_count ; ++i){
        
        uint32_t addr = (uint32_t)tar_begin;
        kpfree(addr + i*0x1000);
    }    


    vfs_mount("/", ramfs);
    vfs_mount("/dev", devfs_create());
    vfs_mount("/proc", proc_create());


    fs_node_t* font_file = kopen("/share/screenfonts/consolefont_14.psf", O_RDONLY);
    if(!font_file){
        uart_print(COM1, "Failed to open font\r\n");
        halt();
        }

    
    uint8_t* font_file_buffer = kmalloc(font_file->length);
    if(!font_file_buffer){

        error("failed to allocate space for font file");
    }
        
    read_fs(font_file, 0, font_file->length, font_file_buffer);
    parse_psf( font_file_buffer);
    close_fs(font_file);

    uart_print(COM1, "Framebuffer at %x : %u %u %ux%u\r\n", 
                mbd->framebuffer_addr_lower,
                mbd->framebuffer_bpp,
                mbd->framebuffer_pitch,
                mbd->framebuffer_width,
                mbd->framebuffer_height
                 );
    init_framebuffer((void*)mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height);
    fb_set_console_color( (pixel_t){.blue = 0xff, .red = 0xff, .green = 0xff }, (pixel_t){.blue = 0x61, .red = 0x61, .green = 0x61 });

    //the framebuffer entry
    kdir_entry [mbd->framebuffer_addr_lower >> 22 ] |= PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLED;
    for(size_t start = 0; start <= (mbd->framebuffer_width*mbd->framebuffer_height*4); start+= 0x1000 ){
        set_virtaddr_flags(mbd->framebuffer_addr_lower + start, PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLED | PAGE_READ_WRITE | PAGE_PRESENT);
    }   



    uint32_t console_size =  init_fb_console(-1, -1); //maximum size //requires fonts tho
    uint32_t row,col;

    row = console_size & 0xffff;
    col = console_size >> 16;
    fb_console_printf("init console dimensions: %ux%u\n", col, row);
    uart_print(COM1, "init console %ux%u\r\n", col, row );

    

    pmm_print_usage();    

    

    rsdp_t rsdp = find_rsdt();
    if(!rsdp_is_valid(rsdp)){
        fb_console_put("Failed to find rsdp in the memory, halting...");
        uart_print(COM1, "Failed to find rsdp in the memory, halting...");
        halt();
    }


    RSDT_t * rsdt = (void *)rsdp.RsdtAddress;
    if(!is_virtaddr_mapped(rsdt) ){
        map_virtaddr(rsdt, rsdt, PAGE_PRESENT | PAGE_READ_WRITE);
    }
    
    if(!doSDTChecksum(&rsdt->h)){
        fb_console_printf("Invalid rsdt :(\n");
        return;
    }
    
    fb_console_printf("other sdts:\n");
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    ACPISDTHeader_t **h = (ACPISDTHeader_t**)&rsdt->PointerToOtherSDT; //it's a table u idiot
    for(int i = 0; i < entries; ++i){
        
        char signature[5] = {0};
        memcpy(signature, h[i]->Signature, 4);
        fb_console_printf("->%u : %s\n", i, signature);
    }


    ACPISDTHeader_t* madt = find_sdt_by_signature(rsdt, "APIC");
    if(madt){
        int result = acpi_parse_madt(madt);
        if(result != 0){
            fb_console_printf("Failed to parse madt table\n");
            halt();
        }
    }
    

   
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


    fb_console_printf("Initializing Device Manager\n");
    dev_init();
    

    timer_install();
    timer_phase(1000);
    
    enumerate_pci_devices();
    

    fb_console_printf("Found PCI devices: %u\n", pci_devices.size);
    for(listnode_t * node = pci_devices.head; node != NULL ;node = node->next ){
        pci_device_t *dev = node->val;

        fb_console_printf("pci-> %u:%u:%u class_code:%x interrupt_line:%u interrupt_pin:%u\n",dev->bus, dev->slot, dev->func, dev->header.common_header.class_code ,dev->header.type_0.interrupt_line, dev->header.type_0.interrupt_pin);        
        pci_list_capabilities(dev);

        if( dev->header.common_header.vendor_id == 0x1af4){
            virtio_register_device(dev);
        }
        
        if( dev->header.common_header.class_code == PCI_NETWORK){
            //we should check if we have properiate driver for this device
            if( dev->header.common_header.vendor_id == 0x8086 &&
                dev->header.common_header.device_id == 0x100e
                )
            {
                fb_console_printf("well an it seems e1000 network device: IRQ number and line -> %x\n", dev->header.type_0.interrupt_line);

            }
        }

        if( dev->header.common_header.class_code == PCI_DISPLAY){
            
            //well check whether this display controoler is our by checking the bar0
            if(dev->header.type_0.base_address_0){
                fb_console_printf("Well well well isn't it our frame buffer at: /pci/%u/%u/%u\n", dev->bus, dev->slot, dev->func);
            }

            int result = bosch_vga_register_device(dev);
            if(result != 0){
                fb_console_put("failed to register bosch vga device\n");
                continue;
            }
        }
        
    }

    

    install_tty();
    install_kernel_mem_devices();
    ata_find_devices();
    ps2_kbd_initialize();
    ps2_mouse_initialize();


  
    device_t* console = kcalloc(1, sizeof(device_t));
    console->name = strdup("console");
    console->write = console_write;
    console->read =  console_read;
    console->dev_type = DEVICE_CHAR;
    console->unique_id = 2;
    dev_register(console);


    dev_register( create_uart_device(COM1) );
    install_basic_framebuffer(mbd->framebuffer_addr_lower, mbd->framebuffer_width, mbd->framebuffer_height, mbd->framebuffer_bpp);

    

    install_pex();

    process_init();
    pcb_t * init =  create_process("/bin/init", (char* []){"/bin/init", NULL} );
    init->parent = NULL;


    register_timer_callback(schedule);
    enable_interrupts();    
    jump_usermode(); //thus calling schedular

    
    //we shouldn't get there //if we do.. well that's imperessive
    for(;;)
    {
    
    }

}
