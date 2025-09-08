#ifndef __PCI_H__
#define __PCI_H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>
#include <g_list.h>
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_MAX_BUS        256
#define PCI_MAX_DEVICE     32
#define PCI_MAX_FUNCTION   8



typedef struct {
    u16       vendor_id; u16 device_id;
    u16     command_reg; u16 status;
    u8      revision_id; u8 programming_if; u8  subclass; u8 class_code;
    u8  cache_line_size; u8  latency_timer; u8  header_type; u8 built_in_self_test;

}__attribute__((packed)) pci_device_common_header_t;

typedef struct {
    u32 base_address_0;
    u32 base_address_1;
    u32 base_address_2;
    u32 base_address_3;
    u32 base_address_4;
    u32 base_address_5;
    u32 card_bus_cis_pointer;
    u16 sub_system_id; u16 sub_system_vendor_id;
    u32 expansion_rom_base_address;
    u8 capabilities_pntr; u8 _reserved[3];
    u32 __reserved;
    u8  interrupt_line; u8 interrupt_pin; u8 min_grant; u8 max_latency;

}__attribute__((packed)) pci_device_type_0_t;


typedef struct {
    uint32_t base_address_0;
    uint32_t base_address_1;
    
    uint8_t primary_bus_number;
    uint8_t second_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t secondary_latency_timer;

    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;

    uint16_t memory_base;
    uint16_t memory_limit;

    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;

    uint32_t prefetchable_base_upper_32_bit;
    uint32_t prefetchable_limit_upper_32_bit;

    uint16_t io_base_upper_16_bit;
    uint16_t io_limit_upper_16_bit;

    u8 capabilities_pntr; u8 _reserved[3];
    uint32_t expansion_rom_base_address;
    uint8_t  interrupt_line; uint8_t interrupt_pin; uint8_t min_grant; uint8_t bridge_control;

}__attribute__((packed)) pci_device_type_1_t;


enum pci_capability_id {
    PCI_NULL= 0,
    PCI_PCI_POWER,
    PCI_AGP,
    PCI_VPD ,
    PCI_SLOT_ID,
    PCI_MSI,
    PCI_COMPACT_PCI,
    PCI_PCI_X,
    PCI_HYPER_TRANSPORT,
    PCI_VENDOR_SPECIFIC,
    PCI_DEBUG_PORT,
    PCI_COMPACT_PCI_RESOURCE_CTRL,
    PCI_PCI_HOT_PLUG,    
    PCI_BRIDGE_SUBSYS,
    PCI_AGP_8X,
    PCI_SECURE_DEVICE,
    PCI_PCI_EXPRESS,
    PCI_MSI_X,
    PCI_SERIAL_ATA
};

enum pci_class{
    PCI_UNCLASSIFIED = 0,
    PCI_MASS_STORAGE,
    PCI_NETWORK,
    PCI_DISPLAY,
    PCI_MULTIMEDIA,
    PCI_MEMORY,
    PCI_BRIDGE,
    PCI_SIMPLE_COMM,
    PCI_BASE_SYSTEM,
    PCI_INPUT_DEVICE,
    PCI_DOCKING,
    PCI_PROCESSOR,
    PCI_SERIAL_BUS,
    PCI_WIRELESS,
    PCI_INTELLIGENT,
    PCI_SATELLITE,
};

extern const char * pci_capability_str[];


typedef struct {
    
    pci_device_common_header_t common_header;

    union{
        pci_device_type_0_t type_0;
        pci_device_type_1_t type_1;
    };

} pci_device_header_t;


typedef struct {
    u8 bus; u8 slot; u8 func;
    pci_device_header_t header;
} pci_device_t;




extern list_t pci_devices;

uint8_t pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void * pci_read(void * dst, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, unsigned int n);

void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void enumerate_pci_devices();
int32_t pci_get_bar_size(pci_device_t dev, unsigned int bar );
int32_t pci_list_capabilities(pci_device_t* dev);
void pci_enable_bus_mastering(pci_device_t* dev);

void recursive_enumerate_pci_devices(uint8_t bus);


#endif