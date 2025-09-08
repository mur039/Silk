#ifndef __BOSCH_VGA_H__
#define __BOSCH_VGA_H__

#include <pmm.h>
#include <pci.h>
#include <sys.h>
#include <dev.h>
#include <uart.h>
#include <fb.h>

typedef struct{
    uint32_t* base_addr;
    uint32_t  size;
    uint32_t* mmio_base;
    uint16_t id; uint16_t _;
} bga_t;

int bosch_vga_register_device(pci_device_t* device);

#endif