#include <pci.h>

unsigned int numberofdevices = 0;
list_t  pci_devices = {.head = 0, .tail = 0, .size = 0};


const char * pci_capability_str[] = {
                         [PCI_NULL] = "PCI_NULL",
                    [PCI_PCI_POWER] = "PCI_POWER",
                          [PCI_AGP] = "PCI_AGP",
                          [PCI_VPD] = "PCI_VPD ",
                      [PCI_SLOT_ID] = "PCI_SLOT_ID",
                          [PCI_MSI] = "PCI_MSI",
                  [PCI_COMPACT_PCI] = "PCI_COMPACT_PCI",
                        [PCI_PCI_X] = "PCI_PCI_X",
              [PCI_HYPER_TRANSPORT] = "PCI_HYPER_TRANSPORT",
              [PCI_VENDOR_SPECIFIC] = "VENDOR_SPECIFIC",
                   [PCI_DEBUG_PORT] = "PCI_DEBUG_PORT",
    [PCI_COMPACT_PCI_RESOURCE_CTRL] = "PCI_COMPACT_PCI_RESOURCE_CTRL",
                 [PCI_PCI_HOT_PLUG] = "PCI_HOT_PLUG" ,
                [PCI_BRIDGE_SUBSYS] = "PCI_BRIDGE_SUBSYS",
                       [PCI_AGP_8X] = "PCI_AGP_8X",
                [PCI_SECURE_DEVICE] = "PCI_SECURE_DEVICE",
                  [PCI_PCI_EXPRESS] = "PCI_PCI_EXPRESS",
                        [PCI_MSI_X] = "PCI_MSI_X",
                   [PCI_SERIAL_ATA] = "PCI_SERIAL_AT"
};

const char * class_str[] = {
    [0x0] = "Unclassified Device",
    [0x1] = "Mass Storage Device",
    [0x2] = "Network Controller",
    [0x3] = "Display Controller",
    [0x4] = "Multimedia Controller",
    [0x5] = "Memory Controller",
    [0x6] = "Bridge Device",
    [0x7] = "Simple Communication Controller",
    [0x8] = "Base System Peripheral",
    [0x9] = "Input Device Controller",
    [0xa] = "Docking Station",
    [0xb] = "Processor", //huh?
    [0xc] = "Serial Bus Controller",
    [0xd] = "Wireless Controller",
    [0xe] = "Intelligent Controller",
    [0xf] = "Satellite Communicatoin Controller",
};

// Read a 32-bit value from the PCI configuration space
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create the configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                         (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    // Write the address to the PCI configuration address port
    outl(PCI_CONFIG_ADDRESS, address);

    // Read the data from the PCI configuration data port
    uint32_t data;
    data = inl(PCI_CONFIG_DATA);
    return data;
}

// Read a 8-bit value from the PCI configuration space
uint8_t pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create the configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                         (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    // Write the address to the PCI configuration address port
    outl(PCI_CONFIG_ADDRESS, address);

    // Read the data from the PCI configuration data port
    uint32_t data;
    data = inb(PCI_CONFIG_DATA);
    return data;
}

void * pci_read(void * dst, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, unsigned int n){
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    u32 * head = dst;

    //for now i will read in 4 bytes
    int full, partial;
    partial = n % 4; // 0..3
    full = (n - partial) / 4;

    
    for(int i = 0; i < full; ++i){
        // Create the configuration address
        uint32_t data = pci_read_config(bus, slot, func, offset);
        head[i] = data;
        offset += 4;
    }

    if(partial){
        offset += 4;
        u32 data = pci_read_config(bus, slot, func, offset);
        u32 mask = 0x00fffffful  >> (( 3 - partial) * 8) | 0ul; // 0:
        head[(full / 4) + 1] = data & mask;
    }

    return dst;
}






// write a 32-bit value to the PCI configuration space
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value){
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    // Create the configuration address
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                         (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    // Write the address to the PCI configuration address port
    outl(PCI_CONFIG_ADDRESS, address);

    // Write the data to the PCI configuration data port
    outl(PCI_CONFIG_DATA, value);
    return;
}



// Enumerate all PCI devices
void enumerate_pci_devices() {


    uint8_t bus, device, function;
    for (bus = 0; bus < PCI_MAX_BUS - 1; bus++) {

        for (device = 0; device < PCI_MAX_DEVICE; device++) {

            for (function = 0; function < PCI_MAX_FUNCTION; function++) {
                



                /*
                    */
                pci_device_header_t pci;
                for(unsigned int i = 0; i < 4 ; ++i){
                    u32 * head = (u32*)&pci;
                    head[i] = pci_read_config(bus, device, function, i * 4);
                }
                if (pci.common_header.vendor_id != 0xFFFF) { // Device present

                    for(unsigned int i = 0; i < (sizeof(pci_device_t)/4) ; ++i){
                        u32 * head = (u32*)&pci;
                        head[i] = pci_read_config(bus, device, function, i * 4 );
                    }


                    pci_device_t * p = kcalloc(1, sizeof(pci_device_t));

                    p->bus = bus;
                    p->slot = device;
                    p->func = function;
                    p->header = pci;
                    list_insert_end(&pci_devices, p);

                    uart_print( COM1, "Found PCI device: bus %x, device %x, function %x\r\n", bus, device, function);
                    uart_print( COM1, "Vendor ID: 0x%x, Device ID: 0x%x Header type: 0x%x\r\n", 
                                pci.common_header.vendor_id, 
                                pci.common_header.device_id,
                                pci.common_header.header_type
                                );
                    uart_print( COM1, "Class: %s, Subclass: 0x%x, Prog IF: 0x%x \r\n\r\n", 
                                class_str[pci.common_header.class_code], 
                                pci.common_header.subclass, 
                                pci.common_header.programming_if
                                );




                /*
                uint32_t vendor_device = pci_read_config(bus, device, function, 0);
                uint16_t vendor_id = vendor_device & 0xFFFF;
                uint16_t device_id = (vendor_device >> 16) & 0xFFFF;

                
                if (vendor_id != 0xFFFF) { // Device present
                    uint32_t class_code = pci_read_config(bus, device, function, 8);
                    uint8_t base_class = (class_code >> 24) & 0xFF;
                    uint8_t sub_class = (class_code >> 16) & 0xFF;
                    uint8_t prog_if = (class_code >> 8) & 0xFF;

                    

                    uart_print( COM1, "Found PCI device: bus %x, device %x, function %x\r\n", bus, device, function);
                    uart_print( COM1, "Vendor ID: 0x%x, Device ID: 0x%x\r\n", vendor_id, device_id);
                    uart_print( COM1, "Class: %s, Subclass: 0x%x, Prog IF: 0x%x \r\n\r\n", class_str[base_class], sub_class, prog_if);
                */


                }
            }
        }
    }
}


int32_t get_bar_size(pci_device_t dev, unsigned int bar ){

    if(dev.header.common_header.header_type & 0x7f && bar > 5) return -1;
    

    uint32_t bar_addr = pci_read_config(dev.bus, dev.slot, dev.func, 0x10 + bar*0x4) & ~(0xful); //save bar addr

    pci_write_config(dev.bus, dev.slot, dev.func, 0x10 + bar*0x4, 0xffffffff);
    uint32_t size = pci_read_config(dev.bus, dev.slot, dev.func, 0x10 + bar*0x4) & ~(0xful);
    size = ~size;
    size += 1;
    pci_write_config(dev.bus, dev.slot, dev.func, 0x10 + bar*0x4, bar_addr);

    return size;
}