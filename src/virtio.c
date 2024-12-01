#include <virtio.h>



#define align2_2_byte(val) (((uint32_t)val + 2) & ~2)

static list_t *virtio_devices = NULL;


i32 virtio_gpu_register(pci_device_t *dev){

    fb_console_printf("VirtIO GPU device\n");

    virtio_device_t * vdev = kcalloc(1, sizeof(virtio_device_t) );
    list_insert_end( virtio_devices, vdev);
    
    u32 notify_multiplier = 0, notify_offset = 0;
    int capability_pntr = dev->header.type_0.capabilities_pntr & ~0b11;
    fb_console_printf("capacility_pntr: %x\n", capability_pntr);

    for( ; capability_pntr != 0 ; ){ //linked list

        u8 cap_id, cap_next, len, cfg_type;
        u32 data = pci_read_config(
                                        dev->bus,
                                        dev->slot,
                                        dev->func,
                                        capability_pntr
                                    );
        cap_id = data & 0xff;
        cap_next = (data >> 8) &0xff;
        len = (data >> 16) &0xff;
        cfg_type = (data >> 24) & 0xff;
        
        virtio_pci_cap_t virtio_cap;
        pci_read(&virtio_cap, dev->bus, dev->slot, dev->func, capability_pntr, sizeof(virtio_pci_cap_t) );


        switch (virtio_cap.cfg_type){
        case VIRTIO_PCI_CAP_COMMON_CFG:
            fb_console_printf("Found \"VIRTIO_PCI_CAP_COMMON_CFG:\" \n");
            fb_console_printf(
                "\tBAR%u is %x and offset %u and size %u\n", 
                virtio_cap.bar, 
                ((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111, 
                virtio_cap.offset,
                virtio_cap.length
                );


            virtio_pci_common_cfg_t * c_conf = (void*)(  (((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111) + virtio_cap.offset);
            vdev->common_cfg = c_conf;

            //this will cause a page fault but anyway;
            if(!is_virtaddr_mapped(c_conf)){
                // fb_console_printf(" well address:%x is not mapped so nect line will cause pagefault\n", c_conf);    
                map_virtaddr(c_conf, c_conf, PAGE_PRESENT | PAGE_READ_WRITE);
            }
            
            c_conf->device_status = 0; //reset the device
            c_conf->device_status |= DEVICE_ACKNOWLEDGED;

            c_conf->driver_feature_select = 0;
            c_conf->driver_feature = VIRTIO_GPU_F_EDID;

            c_conf->driver_feature_select = 1;
            // c_conf->driver_feature = 0ul;

            c_conf->device_status |= DRIVER_FEATURES_OK;

            if(!(c_conf->device_status & DRIVER_FEATURES_OK)){
                fb_console_printf("Failed to set features for virtio-gpu device\n");
                return -1;
            }

            fb_console_printf("\tDevice has notify_queue : %u\n", c_conf->queue_notify_off);


            fb_console_printf("Device has %u queues:\n", c_conf->num_queues);
            for(int i = 0; i < c_conf->num_queues; ++i){
                c_conf->queue_select = i;
                fb_console_printf("\tQueue %u: size: %u\n", i, c_conf->queue_size);
            }
    
            break;

        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            fb_console_printf("Found \"VIRTIO_PCI_CAP_NOTIFY_CFG:\" \n");
            fb_console_printf(
                "\tBAR%u is %x and offset %u and size %u\n", 
                virtio_cap.bar, 
                ((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111, 
                virtio_cap.offset,
                virtio_cap.length
                );

            virtio_pci_notify_cap_t cf;
            pci_read(&cf, dev->bus, dev->slot, dev->func, capability_pntr, virtio_cap.cap_len );
            fb_console_printf("multiplier %u %u\n", virtio_cap.offset, cf.notify_off_multiplier);
            vdev->queue_notify =  cf;
            
            vdev->notify_base_offset_addr = (void *)((((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111) +  virtio_cap.offset);
            


            break;

        case VIRTIO_PCI_CAP_ISR_CFG:
            fb_console_printf("Found \"VIRTIO_PCI_CAP_ISR_CFG:\" \n");
            fb_console_printf(
                "\tBAR%u is %x and offset %u\n", 
                virtio_cap.bar, 
                ((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111, 
                virtio_cap.offset
                );

            break;

        case VIRTIO_PCI_CAP_DEVICE_CFG:
            fb_console_printf("Found \"VIRTIO_PCI_CAP_DEVICE_CFG:\" \n");
            fb_console_printf(
                "\tBAR%u is %x and offset %u\n", 
                virtio_cap.bar, 
                ((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111, 
                virtio_cap.offset
                );

            break;

        case VIRTIO_PCI_CAP_PCI_CFG:
            fb_console_printf("Found \"VIRTIO_PCI_CAP_PCI_CFG:\" \n");
            fb_console_printf(
                "\tBAR%u is %x and offset %u\n", 
                virtio_cap.bar, 
                ((u32*)&dev->header.type_0.base_address_0)[virtio_cap.bar] & ~0b1111, 
                virtio_cap.offset
                );

            
            break;

        
        default: 
            fb_console_printf("Reserved config type %x, ignoring\n", virtio_cap.cfg_type);
            break;
        }
        
        
        


        capability_pntr = cap_next;
    }

    
     //for now i'll implement only queue 0 or controlq
    vdev->common_cfg->queue_select = VIRTIO_GPU_CONTROLQ;        
    int n = vdev->common_cfg->queue_size;

    virt_queue_t ctrlq = virtio_create_virtual_queue(n);
    virtio_gpu_ctrl_hdr_t * ctrl = kcalloc(64, sizeof(virtio_gpu_ctrl_hdr_t) );

    for(int i = 0; i < n; ++i){
        ctrlq.desc_table[i].addr = (u32)get_physaddr(&ctrl[i]);
        ctrlq.desc_table[i].len  = sizeof(virtio_gpu_ctrl_hdr_t);
        ctrlq.desc_table[i].flags = VIRTQ_DESC_F_NEXT | ( (i & 1) * VIRTQ_DESC_F_WRITE ); //make every odd ring device write-only
        ctrlq.desc_table[i].next = (i + 1) % n;
    }

    ctrlq.driver_area->flags = 0;
    ctrlq.driver_area->idx = 0;
    for(u16 i = 0; i < 64; ++i){
        
        (&ctrlq.driver_area->ring)[i] = i;
    }

    ctrlq.device_area->flags = 0;
    ctrlq.device_area->idx = 0;
    for(u16 i = 0; i < 64; ++i){
        virtq_used_elem_t * ueh = &ctrlq.device_area->ring;
        ueh[i].id = 0;
        ueh[i].len = 0;
    }

    vdev->common_cfg->queue_desc   =  ( (u32)get_physaddr(ctrlq.desc_table)  ) >> 12;
    vdev->common_cfg->queue_driver =  ( (u32)get_physaddr(ctrlq.driver_area) ) >> 12;
    vdev->common_cfg->queue_device =  ( (u32)get_physaddr(ctrlq.device_area) ) >> 12;
    vdev->vq = ctrlq;
    vdev->common_cfg->queue_enable = 1;
    vdev->common_cfg->device_status |= DRIVER_LOADED ;
    vdev->common_cfg->device_status |= DRIVER_OK;
    fb_console_printf("Read back from the device :%x\n", vdev->common_cfg->device_status);


    vdev->vq.driver_area->idx += 32; //one for driver one for device
    virtio_gpu_ctrl_hdr_t * msg = ctrl;
    
    msg->type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    msg->ctx_id   = 0;
    msg->fence_id = 0;
    msg->flags    = 0;
    msg->padding  = 0;

    //for virtual queue 0
    // vdev->notify_base_offset_addr[1] = 0;

    for(int i = 0; i < 0xfffff; ++i); //just wail

    // for(int i = 0; i < 64; ++i){
    //     fb_console_printf(
    //         "\t-> %u %u\n",
    //         (&vdev->vq.device_area->ring)[i].id,
    //         (&vdev->vq.device_area->ring)[i].len
    //         );

    // }


    return 0;
}

i32 virtio_register_device( pci_device_t *dev){
    if (!virtio_devices){
        virtio_devices = kcalloc(sizeof(list_t), 1);
        virtio_devices->size = 0;
        virtio_devices->head = 0;
        virtio_devices->tail = 0;

    }
    int device_type = dev->header.common_header.device_id - 0x1040;
    switch (device_type){

    case VIRTIO_GPU_DEVICE:
        return virtio_gpu_register(dev);
        
        break;
        
    default:
        fb_console_printf("Unknown VirtIO device...\n");
        return -1;
        break;
    }
}

virt_queue_t virtio_create_virtual_queue(unsigned int queue_entry_size){

      
    virt_queue_t ret = {.virt_queue_virtual_addr = NULL};
    int n = queue_entry_size;
    u32 desc_size = n * sizeof(virtq_desc_t);
    u32 avail_size = sizeof(virtq_avail_t) + (n * sizeof(u16));
    u32 used_size = 6  + (n * sizeof(virtq_used_elem_t));
    u32 total_size = (desc_size + avail_size + used_size);

    fb_console_printf( 
        " virtio_create_queue n = %u : %u %u %u -> %u\n",
        n,
        desc_size,
        avail_size,
        used_size,
        total_size
    );

    if( total_size > 4095){
        fb_console_printf("Currently not supported total_size > PAGESIZE\n");
        return ret;
    }

    ret.queue_size = n;
    
    ret.virt_queue_virtual_addr = kpalloc(1); //allocate a page, 4kB
    memset(ret.virt_queue_virtual_addr, 0, 4096);

    ret.desc_table = ret.virt_queue_virtual_addr;
    ret.driver_area = (void *)(((u32)ret.desc_table) + desc_size);
    ret.device_area = (void *)(((u32)ret.driver_area) + avail_size);

    
    

    // for(int i = 0; i < n; ++i){
    //     virtq_desc_t * desc = ret.desc_table;

    //     desc[i].addr = get_physical_address(buffer[i]); // DMA-accessible buffer address
    //     desc[i].len = buffer_size;                     // Size of the buffer
    //     desc[i].flags = 0;                             // Default flags (set as needed)
    //     desc[i].next = (i + 1) % 64;                   // Point to the next descriptor (circular)
    // }


    return  ret;
}