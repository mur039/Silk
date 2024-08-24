#include <pci.h>


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
    // __asm__ __volatile__("outl %0, %1" : : "a"(address), "Nd"(PCI_CONFIG_ADDRESS));

    // Read the data from the PCI configuration data port
    uint32_t data;
    data = inl(PCI_CONFIG_DATA);
    // __asm__ __volatile__("inl %1, %0" : "=a"(data) : "Nd"(PCI_CONFIG_DATA));
    return data;
}

// Enumerate all PCI devices
void enumerate_pci_devices() {
    uint8_t bus, device, function;
    for (bus = 0; bus < PCI_MAX_BUS - 1; bus++) {
        for (device = 0; device < PCI_MAX_DEVICE; device++) {
            for (function = 0; function < PCI_MAX_FUNCTION; function++) {
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
                    uart_print( COM1, "Class: 0x%x, Subclass: 0x%x, Prog IF: 0x%x \r\n\r\n", base_class, sub_class, prog_if);
                }
            }
        }
    }
}


