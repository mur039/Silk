#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <network/netif.h>

typedef struct route {
    uint32_t destination;   // Network address
    uint32_t netmask;       // Netmask
    uint32_t gateway;       // Next hop (0.0.0.0 means direct)
    struct nic* interface;   // Outgoing interface
} route_t;

int route_add_route(uint32_t ipaddr, uint32_t netmask, uint32_t gateway, struct nic* iface);
void route_remove_routes_for_interface(struct nic* dev);
struct nic* route_select_interface(uint32_t dest_ip);
const route_t* route_select_route(uint32_t dest_ip);

#endif