#include "network/e1000.h"
#include "pmm.h"
#include "pmm.h"
#include "fb.h"
#include "irq.h"


struct arena_alloc{
    void* _rawptr;
    size_t _allocsize;
    size_t _allocindex;
};

struct arena_alloc arena_initialize(size_t block_size){
    struct arena_alloc retval = {._rawptr = NULL, ._allocsize = 0, ._allocindex = 0};
    void* ptr = kcalloc(1, block_size);
    if(!ptr){
        return retval;
    }

    retval._rawptr = ptr;
    retval._allocsize = block_size;
    retval._allocindex = 0;
    
    return retval;
}

void* arena_allocate( struct arena_alloc *block, size_t size){

    size_t remaining = block->_allocsize - block->_allocindex;
    if( remaining < size) 
        return NULL;
    
    uint8_t *byteptr = block->_rawptr;
    byteptr += block->_allocindex;
    block->_allocindex += size;
    return byteptr;
}

void arena_reset(struct arena_alloc* block){
    block->_allocindex = 0;
    return;
}

//package builder
static struct arena_alloc package_builder;

static struct nic *e1000_nic = NULL;


#define RX_DESC_COUNT  32
struct e1000_rx_desc rx_descs[RX_DESC_COUNT];
uint8_t rx_bufs[RX_DESC_COUNT][2048];

static void initialize_rx_e1000(pci_device_t* pci){
    
    volatile uint32_t* regs = (volatile uint32_t*)(pci->header.type_0.base_address_0 & ~0xful);
   
    //register the descriptors
    for (int i = 0; i < RX_DESC_COUNT; ++i) {
        rx_descs[i].addr = (uint64_t)(uint32_t)get_physaddr(&rx_bufs[i]);
        rx_descs[i].status = 0;
    }


    regs[E1000_REG_RDBAL / 4] = (uint32_t)get_physaddr(rx_descs);
    regs[E1000_REG_RDBAH / 4] = 0;
    regs[E1000_REG_RDLEN / 4] = RX_DESC_COUNT* sizeof(struct e1000_rx_desc);
    regs[E1000_REG_RDH / 4] = 0;
    regs[E1000_REG_RDT / 4] = RX_DESC_COUNT - 1;

    regs[E1000_REG_RCTL / 4] =  RCTL_EN |  RCTL_BAM |  RCTL_UPE |  RCTL_MPE |  RCTL_SECRC;
    

    //enable rxint
    regs[E1000_REG_IMS / 4] = 0x1F6DC;
    return;
}


//transmit buffers
#define TX_DESC_COUNT  8
struct e1000_tx_desc tx_descs[TX_DESC_COUNT];
uint8_t tx_bufs[TX_DESC_COUNT][1518];

static void initialize_tx_e1000(pci_device_t* pci){
    
    volatile uint32_t* regs = (volatile uint32_t*)(pci->header.type_0.base_address_0 & ~0xful);
    
    for (int i = 0; i < TX_DESC_COUNT; ++i) {
        tx_descs[i].addr = (uint64_t)(uint32_t)get_physaddr(&tx_bufs[i]);
        tx_descs[i].status = 1; // Mark as done
        tx_descs[i].length = 0; 
        tx_descs[i].cmd = 0; 
    }

    regs[E1000_REG_TDBAL / 4] = (uint32_t)get_physaddr(tx_descs);
    regs[E1000_REG_TDBAH / 4] = 0;

    regs[E1000_REG_TDLEN / 4] = TX_DESC_COUNT*sizeof(struct e1000_tx_desc);

    regs[E1000_REG_TDH / 4] = 0;
    regs[E1000_REG_TDT / 4] = 0;

    regs[E1000_REG_TCTL / 4] = 0x00000002 | 0x00000008 | (0x40 << 12) | (0x10 << 4); // EN | PSP | CT | COLD
    regs[E1000_REG_TIPG / 4] = (10 << 20) | (8 << 10) | 6;  // default values for full-duplex
    return;
}




volatile uint32_t* e1000_regs = NULL;
uint32_t current_rx = 0;
uint8_t local_macaddress[6] = {0};
uint8_t myip[4] = {0};

static void e1000e_int_handler(struct regs* r){
    
    uint32_t icr = e1000_regs[E1000_REG_ICR / 4]; // Acknowledge

    while (rx_descs[current_rx].status & 0x01) {

        uint8_t *data = rx_bufs[current_rx];
        uint16_t len = rx_descs[current_rx].length;

        struct eth_frame* frame = (struct eth_frame*)data;
        e1000_nic->handle_rx(e1000_nic, frame, len);
        
        rx_descs[current_rx].status = 0;
        e1000_regs[E1000_REG_RDT / 4] = current_rx;
        current_rx = (current_rx + 1) % RX_DESC_COUNT;

        arena_reset(&package_builder);
    }

    return;
}


int initialize_e1000(pci_device_t* pcidev){

    struct nic *nic = netif_allocate();
    if(!nic){
        fb_console_printf("No nic left exiting");
        return 1;
    }

    e1000_nic = nic;
    fb_console_printf("Well well well an e1000 device i see\n");
    for(int i = 0;  i< 6; ++i){
        uint32_t* bars = (uint32_t*)&pcidev->header.type_0.base_address_0;
        if(bars[i]){

            fb_console_printf("BAR%u: %s -> %x\n", i, bars[i] & 1 ? "I/O" : "MMIO", bars[i] & ~1ul);
        }
    }

    //we need to retrieve and map the address in bar0
    size_t regs_size =  pci_get_bar_size(*pcidev, 0);
    e1000_regs = (uint32_t*)(pcidev->header.type_0.base_address_0 & ~0xful);

    fb_console_printf("BAR0 address:%x and size:%x\n", e1000_regs, regs_size);
    for(size_t i = 0; i < regs_size; i +=  4096){
        uint8_t* addr = (uint8_t*)e1000_regs;
        map_virtaddr(addr + i, addr + i, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLED);
    }

    pci_enable_bus_mastering(pcidev);
    
 
    volatile uint32_t* regs = e1000_regs;
    regs[E1000_REG_CTRL / 4] |= (1 << 26); // Device Reset
    io_wait();

    initialize_rx_e1000(pcidev);
    initialize_tx_e1000(pcidev);

    e1000_read_MAC(pcidev, local_macaddress);

    int irq = pcidev->header.type_0.interrupt_line;

    irq_install_handler(irq, e1000e_int_handler);

    //there initialize the package builder
    package_builder = arena_initialize(2048);


    //register interface
    struct e1000_priv* priv = kcalloc(1, sizeof(struct e1000_priv));
    priv->mmio_base = e1000_regs;
    priv->pci = pcidev;
    priv->rx_bufs = (uint8_t**)rx_bufs;
    priv->tx_bufs = (uint8_t**)tx_bufs;
    priv->tx_buf_count = TX_DESC_COUNT;
    priv->rx_buf_count = RX_DESC_COUNT;

    sprintf(nic->name, "eth0");
    nic->priv = priv;
    nic->handle_rx = net_receive;
    nic->send = e1000_nic_send;
    nic->poll = NULL;
    nic->mtu = 2048;
    e1000_read_MAC(pcidev, nic->mac);
    
    return 0;

}

int e1000_read_MAC(pci_device_t* pcidev, uint8_t* mac){

    volatile uint32_t* regs = (volatile uint32_t*)(pcidev->header.type_0.base_address_0 & ~0xful);
    uint32_t ral = regs[E1000_REG_RAL/4];
    uint32_t rah = regs[E1000_REG_RAH/4];

    mac[0] = ral & 0xFF;
    mac[1] = (ral >> 8) & 0xFF;
    mac[2] = (ral >> 16) & 0xFF;
    mac[3] = (ral >> 24) & 0xFF;
    mac[4] = rah & 0xFF;
    mac[5] = (rah >> 8) & 0xFF;

    return 0;
}



int e1000_nic_send( struct nic* dev, const void *data, size_t len) {
    struct e1000_priv* priv = dev->priv;
    return e1000_send(priv->mmio_base, (uint8_t*)data, len);
}


int e1000_send(volatile uint32_t* regs, uint8_t *data, size_t len) {

    uint32_t tdt = regs[E1000_REG_TDT / 4];
    uint32_t oldtdt = tdt;
    struct e1000_tx_desc *desc = &tx_descs[tdt];

    if (!(desc->status & 0x1)){
        fb_console_printf("Descriptor is busy\n");
        return -1;
    }


    memcpy(tx_bufs[tdt], data, len);
    desc->length = len;
    desc->cmd = (1 << 3) | ( 1 << 0) ;
    desc->status = 0;

    regs[E1000_REG_TDT / 4] = (tdt + 1) % TX_DESC_COUNT;
    return oldtdt;
}

int e1000_recv(volatile uint32_t* regs, uint8_t *buf, size_t max_len) {
    static int rdt = RX_DESC_COUNT - 1;
    int next = (rdt + 1) % RX_DESC_COUNT;

    if (!(rx_descs[next].status & 0x1)) return 0; // No packet

    size_t len = rx_descs[next].length;
    if (len > max_len) len = max_len;
    memcpy(buf, rx_bufs[next], len);

    rx_descs[next].status = 0;
    regs[E1000_REG_RDT / 4] = next;
    rdt = next;

    return len;
}



// void send_udp_package(uint8_t* target_mac, uint8_t* ipaddress, uint16_t sourceport, uint16_t desport, uint8_t* data, uint16_t data_length){
    

//     //before doing any schietze let's check if given ip adress is in multi-cast region
    
//     // Convert IP to host-order for easier checking
//     uint32_t ip = (ipaddress[0] << 24) | (ipaddress[1] << 16) | (ipaddress[2] << 8) | ipaddress[3];

    
//     uint8_t multicast_mac[6]; //if ip is muticast
//     // Multicast IP range: 224.0.0.0 to 239.255.255.255
//     if ((ip & 0xF0000000) == 0xE0000000) {
        
//         // It's a multicast IP
//         multicast_mac[0] = 0x01;
//         multicast_mac[1] = 0x00;
//         multicast_mac[2] = 0x5e;
//         multicast_mac[3] = ipaddress[1] & 0x7F;  // lower 7 bits of second byte
//         multicast_mac[4] = ipaddress[2];
//         multicast_mac[5] = ipaddress[3];

//         // Replace target_mac with multicast_mac
//         target_mac = multicast_mac;
//     }


//     /*
//         +--------------+
//         |  ETH FRAME   |
//         +--------------+
//         | IPV4 FRAME   |
//         +--------------+
//         |  UDP FRAME   |
//         +--------------+
//     */

//     size_t total_length = sizeof(struct eth_frame) + sizeof(struct ipv4_packet) + sizeof(struct udp_packet) + data_length;
    
//     struct eth_frame *eth= arena_allocate(&package_builder, total_length);//kcalloc(1, total_length);
//     struct ipv4_packet *ipv4 = (struct ipv4_packet* )eth->payload;
//     struct udp_packet *udp = (struct udp_packet*)ipv4->payload;
//     uint8_t* udp_data = udp->data;

//     //constract the frames
//     //eth:
//     memcpy(eth->src_mac, local_macaddress, 6);
//     memcpy(eth->dst_mac, target_mac, 6); //for know we know target mac
//     eth->ethertype = htons(ETHERFRAME_IPV4);

//     ipv4->version_ihl = 0x45;
//     ipv4->TOS = 0;
//     ipv4->total_length = htons(sizeof(struct ipv4_packet) + sizeof(struct udp_packet) + data_length);
//     ipv4->identification = htons(ip_ident_counter++);

//     uint8_t flags = 0x02 << 5;
//     uint16_t frag_offset = 0;
//     ipv4->flags_fragoffset = (flags | (frag_offset << 3));

//     ipv4->ttl = 64;
//     ipv4->protocol = IPV4_PROTOCOL_UDP;

//     memcpy(ipv4->tpa, ipaddress, 4);
//     memcpy(ipv4->spa, myip, 4);

//     ipv4->header_checksum = 0;
//     ipv4->header_checksum = compute_checksum((uint16_t*)ipv4, sizeof(struct ipv4_packet));

//     udp->dport = htons(desport);
//     udp->sport = htons(sourceport);
//     udp->length = htons(sizeof(struct udp_packet) + data_length);
//     udp->checksum = 0; //for ipv4 don't use checksum
//     memcpy(udp->data, data, data_length);


//     udp_calculate_checksum(udp, ipaddress, myip);
    
//     e1000_send(e1000_regs, (uint8_t*)eth, total_length);
//     arena_reset(&package_builder);
//     return;
// }
