#include <bosch_vga.h>


int bosch_vga_register_device(pci_device_t* device){
    
    //altough it's checked during enumuration, there's no harm from sanity checking

    if(!device){
        return -1; //like nigga, nullptr? really?
    }

    int class_code = device->header.common_header.class_code;
    int vendor_id = device->header.common_header.vendor_id;
    int device_id = device->header.common_header.device_id;

    if( !(  class_code == PCI_DISPLAY && vendor_id == 0x1234 && device_id == 0x1111) ){

        return -2;
    }

    fb_console_printf("Well a BGA compatible display controller here\r\n");

    uint32_t* ptr =  (uint32_t*)&device->header;
    for(size_t i = 0; i < sizeof(pci_device_header_t) / 4 ; ++i){
        ptr[i] = pci_read_config(device->bus, device->slot, device->func, i * 4);
    }

    
    fb_console_printf("BAR0 framebuffer addr: %x size: %x\n", device->header.type_0.base_address_0, pci_get_bar_size(*device, 0));
    fb_console_printf("BAR1 Reserved, if 64-bit.\n");
    fb_console_printf("BAR2 MMIO addr: %x size: %x\n", device->header.type_0.base_address_2, pci_get_bar_size(*device, 2));

    int32_t size = pci_get_bar_size(*device, 0);
    uint32_t* base_addr, *mmio_addr;
    base_addr = (uint32_t*)(device->header.type_0.base_address_0 & 0xFFFFFFF0);
    mmio_addr = (uint32_t*)(device->header.type_0.base_address_2 & 0xFFFFFFF0);
    




    return 0;
}