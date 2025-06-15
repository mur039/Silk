#ifndef _E1000_H__
#define _E1000_H__

#include "pci.h"
#include <socket.h>
#include <network/netif.h>

#define E1000_REG_CTRL      0x0000
#define E1000_REG_STATUS    0x0008
#define E1000_REG_EERD      0x0014
#define E1000_REG_ICR       0x00C0
#define E1000_REG_IMS       0x00D0
#define E1000_REG_RCTL      0x0100
#define E1000_REG_TCTL      0x0400
#define E1000_REG_RDBAL     0x2800
#define E1000_REG_RDBAH     0x2804
#define E1000_REG_RDLEN     0x2808
#define E1000_REG_RDH       0x2810
#define E1000_REG_RDT       0x2818
#define E1000_REG_TDBAL     0x3800
#define E1000_REG_TDBAH     0x3804
#define E1000_REG_TDLEN     0x3808
#define E1000_REG_TDH       0x3810
#define E1000_REG_TDT       0x3818

#define E1000_REG_RAL	0x5400	//4	Lower 4 bytes
#define E1000_REG_RAH	0x5404	//4	Upper 2 bytes + flags
#define E1000_REG_TIPG  0x0410

#define RCTL_EN     (1 << 1)
#define RCTL_BAM    (1 << 15)  // Broadcast Accept
#define RCTL_UPE    (1 << 3)   // Unicast Promisc
#define RCTL_MPE    (1 << 4)   // Multicast Promisc
#define RCTL_SECRC  (1 << 26)  // Strip Ethernet CRC





struct e1000_priv{

    volatile void* mmio_base;
    pci_device_t *pci;
    uint32_t tx_buf_count;
    uint32_t rx_buf_count;
    uint8_t** tx_bufs;
    uint8_t** rx_bufs;
};


struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));


struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));


int initialize_e1000(pci_device_t* pcidev);
int e1000_read_MAC(pci_device_t* pcidev, uint8_t* dest);
int e1000_send(volatile uint32_t* regs, uint8_t *data, size_t len);

int e1000_nic_send( struct nic* dev, const void *data, size_t len);

#endif
