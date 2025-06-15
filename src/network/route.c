#include "network/route.h"
#include <sys.h>
#include <fb.h>
#include <pmm.h>

#define ROUTE_MAX_COUNT 8
route_t routes[ROUTE_MAX_COUNT];

int route_add_route(uint32_t ipaddr, uint32_t netmask, uint32_t gateway, struct nic* iface){
    //fine emoty entry in routing table
    for(int i = 0; i < ROUTE_MAX_COUNT; ++i){
        if(!routes[i].interface){

            routes[i].destination = ipaddr;
            routes[i].netmask = netmask;
            routes[i].gateway = gateway;
            routes[i].interface = iface;
            return 1;
        }
    }

    return 0;
}

void route_remove_routes_for_interface(struct nic* dev){

    //remove them john
    for(int i = 0; i < ROUTE_MAX_COUNT; ++i){
        if(routes[i].interface == dev){
            memset(&routes[i], 0, sizeof( route_t));
        }
    }

    return;
}

struct nic* route_select_interface(uint32_t dest_ip) {
    route_t* best_match = NULL;

    for (int i = 0; i < ROUTE_MAX_COUNT; ++i) {
        route_t* route = &routes[i];
        
        if ((dest_ip & route->netmask) == (route->destination & route->netmask)) {
            if (!best_match || route->netmask > best_match->netmask) {
                best_match = route;
            }
        }
    }

    return best_match ? best_match->interface : NULL;
}


const route_t* route_select_route(uint32_t dest_ip) {
    route_t* best_match = NULL;

    for (int i = 0; i < ROUTE_MAX_COUNT; ++i) {
        route_t* route = &routes[i];
        
        if ((dest_ip & route->netmask) == (route->destination & route->netmask)) {
            if (!best_match || route->netmask > best_match->netmask) {
                best_match = route;
            }
        }
    }

    if(!best_match)
        return NULL;
    
    if(!best_match->interface)
        return NULL;

        
    return best_match;
}
