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
#include <filesystems/tar.h>
#include <filesystems/vfs.h>
#include <filesystems/nulldev.h>
#include <filesystems/proc.h>
#include <filesystems/tmpfs.h>
#include <filesystems/ata.h>
#include <filesystems/fat.h>
#include <filesystems/ext2.h>


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


circular_buffer_t * ksock_buf = NULL;
extern void jump_usermode();

static inline void reboot_by_ps2(){ //there's no way i can return
    ps2_send_command(0xD1); //write next byte to output port of the controller
    ps2_send_data(0xFE);
}



volatile int  is_received_char = 0;
char ch;

volatile int  is_kbd_pressed = 0;
char kb_ch;
uint8_t kbd_scancode;


semaphore_t * semaphore_uart_handler = NULL;

void keyboard_handler(struct regs *r)
{
    
  if(r->err_code == 0){;}
    unsigned char scancode;
    scancode = inb(0x60);

    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    /*
    ctrl 0x1d
    alt 0x38
    left shift 0x2a
    right shift 0x36

    */
    if (scancode & 0x80) //released
    {
        if((scancode &  0x7f) == 0x1d){ //if ctrl is released
            ctrl_flag = 0;
        }else if((scancode &  0x7f) == 0x2a || (scancode &  0x7f) == 0x36 ){ //shift modifier
            shift_flag = 0;
        }
        else if( ( scancode & 0x7f) == 0x38 ){ //alt_gr
            alt_gr_flag = 0;
        }
        /* You can use this one to see if the user released the
        *  shift, alt, or control keys... */
    }
    else //pressed
    {

        kbd_scancode = scancode & 0x7f;
        switch(scancode & 0x7f){
            
            //ctrl
            case 0x1d: 
                ctrl_flag = 1; 
                break;

            //both rshift and lshift
            case 0x2a:
            case 0x36:
                shift_flag = 1;
                break;
            
            //alt_gr
            case 0x38:
                alt_gr_flag = 1;
                break;

            //rest of the keys
            default:
                is_kbd_pressed = 1;
                kb_ch = kbdus[scancode & 0x7f];

                // fb_console_printf("kbd_driver : %x ->  %x/%c\n", scancode & 0x7f, kb_ch, kb_ch);

                if(ctrl_flag) 
                    kb_ch &= 0x1f;
                
                if(shift_flag){
                    
                    if(kb_ch == '<'){
                        
                        kb_ch += 2;
                    }
                    else if(kb_ch >= 'a' && kb_ch <= 'z')
                    {

                        kb_ch -= 32;
                    }else if(kb_ch >= 'A' && kb_ch <= 'Z')
                    {

                        kb_ch += 32;
                    }
                }

                if(alt_gr_flag){
                    if(kb_ch == '<'){
                        kb_ch = '|';
                    }
                }

            
                break;
        }

    }
}










uint32_t kernel_stack[2048];
uint32_t kernel_syscallstack[256];


static void in_kernel_yield(struct regs* r){
    save_context(r, current_process);
    schedule(r);
    return;
}


void vfs_traverse(fs_node_t* root_node, int depth){

    char* spacing = kcalloc(1, depth + 1);
    for(int i = 0; i < depth; ++i)
        spacing[i] = ' ';
    
    
    
     while(1){

        int old_offset = root_node->offset;
        struct dirent* item = readdir_fs(root_node, root_node->offset);
        
        if(old_offset > root_node->offset){
            break;
        }

        if(item){
            switch(item->type){
                case FS_FILE:
                    fb_console_printf("%sname : %s> file\n",spacing,  item->name);
                    break;
                
                case FS_DIRECTORY:
                    fb_console_printf("%sname : %s> dir\n",spacing, item->name);
                    fs_node_t* dnode = finddir_fs(root_node, item->name);
                    
                    

                    if(!strcmp(root_node->name, dnode->name)){ return;}

                    vfs_traverse( dnode, depth + 1);
                    break;

                default:break;
            }
			// char* name = strdup(item->name);
			// list_insert_end(list, name);
        	kfree(item);
        }
        else{
            break;
        }
    }

}



//test functions for ata device

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
    irq_install_handler(UART_IRQ, uart_handler);
    irq_install_handler(PS2_MOUSE_IRQ, ps2_mouse_handler);
    // irq_install_handler(ATA_MASTER_IRQ, ata_master_irq_handler);

    //for in kernel yield for kernel_threads
    // idt_set_gate(0x81, (unsigned)in_kernel_yield, 0x08, 0xEE);//0x8E); //syscall


    //fiz paging problems
    virt_address_t addr;
    addr.address = (uint32_t)kernel_heap;
    addr.table += 1;
    for( ; addr.table < 1023; addr.table++){
    //   uart_print(COM1, "incremented table %u:%x , physical :%x\r\n",addr.table,  addr.address, get_physaddr((void *)addr.address));
      unmap_virtaddr((void *)addr.address);
    } 

    
    addr.address = (uint32_t)get_physaddr(kernel_heap);
    for(unsigned int i = 0; i < 1 + (addr.address - 0x100000)/4096 ; ++i){
        pmm_mark_allocated((void *)0x100000 + 0x1000*i);
    }



    for(unsigned int i = 0; i < (modules[0].mod_end - modules[0].mod_start)/4096 ; ++i){
        
        pmm_mark_allocated((void *)modules[0].mod_start + i*0x1000);
    }
    flush_tlb();


    pmm_print_usage();
    kmalloc_init(100);
    alloc_print_list();
    
    
    //turn it into to driver of sort?
    void * tar_begin = (void*)kernel_heap;
    size_t tar_size = (modules[0].mod_end - modules[0].mod_start);

    tar_add_source(tar_begin);
    for(unsigned int i = 0; i < tar_size/4096 + (tar_size % 4096 == 0) ; ++i){
        map_virtaddr(kernel_heap, (void *)(modules->mod_start + i*0x1000), PAGE_PRESENT | PAGE_READ_WRITE);
        kernel_heap += 0x1000;
    }    


    vfs_install();
  
    vfs_mount("/", tar_node_create(tar_begin, tar_size));
    vfs_mount("/dev", devfs_create());
    vfs_mount("/proc", proc_create());




    fs_node_t* font_file = kopen("/share/screenfonts/consolefont_14.psf", O_RDONLY);
    if(!font_file){
        uart_print(COM1, "Failed to open font\r\n");
        halt();
        }

    
    uint8_t* font_file_buffer = kmalloc(font_file->length);
    if(!font_file_buffer)
        error("failed to allocate space for font file");
        
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
    fb_set_console_color( (pixel_t){.blue = 0xff, .red = 0xff, .green = 0xff }, (pixel_t){.blue = 0x00, .red = 0x00, .green = 0x00 });

    uint32_t console_size =  init_fb_console(-1, -1); //maximum size //requires fonts tho
    uint32_t row,col;

    row = console_size & 0xffff;
    col = console_size >> 16;
    fb_console_printf("init console dimensions: %ux%u\n", col, row);    


    pmm_print_usage();    

    

#if 0
    rsdp_t rsdp = find_rsdt();
    if(!rsdp_is_valid(rsdp)){
        fb_console_put("Failed to find rsdp in the memory, halting...");
        uart_print(COM1, "Failed to find rsdp in the memory, halting...");
        halt();
    }


    fb_console_put("RSDP:\n");

    fb_console_put("\tSignature: ");fb_console_putchar('\"');
    for(int i = 0; i < 8; ++i){
        fb_console_putchar(rsdp.Signature[i]);
    }
    fb_console_put("\"\n");


    fb_console_put("\tOEMID: ");fb_console_putchar('\"');
    for(int i = 0; i < 6; ++i){
        fb_console_putchar(rsdp.OEMID[i]);
    }
    fb_console_put("\"\n");


    fb_console_printf("\tRevision: %u\n", rsdp.Revision);
    fb_console_printf("\tRsdt: %x\n", rsdp.RsdtAddress);

    RSDT_t * rsdt = (void *)rsdp.RsdtAddress;
    if(!is_virtaddr_mapped(rsdt) ){
        map_virtaddr(rsdt, rsdt, PAGE_PRESENT | PAGE_READ_WRITE);
    }
    
    if(!doSDTChecksum(&rsdt->h)){
        fb_console_printf("Invalid rsdt :(\n");
        return;
    }
    
    char sig[5], oemid[7], oemtableid[9];
    
    memcpy(sig, rsdt->h.Signature, 4);
    memcpy(oemid, rsdt->h.OEMID, 6);
    memcpy(oemtableid, rsdt->h.OEMTableID, 8);

    fb_console_printf(
        "RSDT:\n"
        "\tsignature:%s\n"
        "\tlength:%u\n"
        "\trevision:%u\n"
        "\tchecksum:%u\n"
        "\toemid:%s\n"
        "\toemtableid:%s\n"
        "\tcreatorid:%x\n"
        "\tcreatorrevision:%x\n"
        , 
        sig,
        rsdt->h.Length,
        rsdt->h.Revision,
        rsdt->h.Checksum,
        oemid,
        oemtableid,
        rsdt->h.CreatorID,
        rsdt->h.CreatorRevision

    );

    fb_console_printf("other sdts:\n");
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    ACPISDTHeader_t *h = (ACPISDTHeader_t*)rsdt->PointerToOtherSDT;
    for(int i = 0; i < entries; ++i){
        fb_console_printf("->%u : %s\n", i, h[i].Signature);
    }

#endif

    

    fb_console_printf("Initializing Device Manager\n");
    dev_init();
    
    initialize_syscalls();
    install_syscall_handler(  SYSCALL_OPEN, syscall_open);
    install_syscall_handler(  SYSCALL_EXIT, syscall_exit);
    install_syscall_handler(  SYSCALL_READ, syscall_read);
    install_syscall_handler( SYSCALL_WRITE, syscall_write);
    install_syscall_handler( SYSCALL_CLOSE, syscall_close);
    install_syscall_handler( SYSCALL_LSEEK, syscall_lseek);
    install_syscall_handler(SYSCALL_EXECVE, syscall_execve);
    install_syscall_handler( SYSCALL_FSTAT, syscall_fstat);
    install_syscall_handler(  SYSCALL_FORK, syscall_fork);
    install_syscall_handler(  SYSCALL_DUP2, syscall_dup2);
    install_syscall_handler(SYSCALL_GETPID, syscall_getpid);
    install_syscall_handler(  SYSCALL_PIPE, syscall_pipe);
    install_syscall_handler( SYSCALL_WAIT4, syscall_wait4);
    install_syscall_handler(  SYSCALL_MMAP, syscall_mmap);
    install_syscall_handler(  SYSCALL_KILL, syscall_kill);
    install_syscall_handler(SYSCALL_GETDENTS, syscall_getdents);
    install_syscall_handler(SYSCALL_GETCWD, syscall_getcwd);
    install_syscall_handler(SYSCALL_CHDIR, syscall_chdir);
    install_syscall_handler(SYSCALL_SYSINFO, syscall_sysinfo);
    install_syscall_handler(SYSCALL_MOUNT, syscall_mount);
    install_syscall_handler(SYSCALL_UNLINK, syscall_unlink);
    install_syscall_handler(SYSCALL_MKDIR, syscall_mkdir);

    timer_install();
    timer_phase(1000);
    
    enumerate_pci_devices();
    

    fb_console_printf("Found PCI devices: %u\n", pci_devices.size);
    for(listnode_t * node = pci_devices.head; node != NULL ;node = node->next ){
        pci_device_t *dev = node->val;

        if( dev->header.common_header.vendor_id == 0x1af4){
            virtio_register_device(dev);

        }

        //display controller
        if( dev->header.common_header.class_code == 0x3){
            fb_console_printf("Found display controller its interrupt line %u and interruptDisable :%u\n", 
                                dev->header.type_0.interrupt_line,
                                (dev->header.common_header.command_reg >> 10) & 1
                                );


        }
        

    }


    ps2_mouse_initialize();

    ata_find_devices();
    
    //maybe implement pivot root?
    //forcefully change the root
    // struct vfs_entry* r =  fs_tree->root->value;
    // r->file = NULL;
    // vfs_mount("/", fat_node_create(drive0) );   
  
    device_t* console = kcalloc(1, sizeof(device_t));
    console->name = strdup("console");
    console->write = console_write;
    console->read = console_read;
    console->dev_type = DEVICE_CHAR;
    console->unique_id = 2;
    
    dev_register(console);

    dev_register( create_uart_device(COM1));


    device_t* fb = kcalloc(1, sizeof(device_t));
    fb->name = strdup("fb0");
    fb->write = fb_write;
    fb->dev_type = DEVICE_BLOCK;
    fb->unique_id = 6;
    dev_register(fb);
        
    process_init();

    char * init_args[] = {
        "/bin/init",
        NULL,
        NULL
    };

    pcb_t * init =  create_process("/bin/init", init_args);
    init->parent = NULL;


    register_timer_callback(schedule);
    enable_interrupts();    
    jump_usermode(); //thus calling schedular

    
    //we shouldn't get there
    for(;;)
    {
    
    }

}
