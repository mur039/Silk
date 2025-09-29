#include <network/netif.h>
#include <network/lo.h>
#include <network/route.h>
#include <network/skb.h>
#include <network/ipv4.h>



int lo_output(struct nic* i_f, struct sk_buff* skb ){
    i_f->send(i_f, skb->data, skb->len);
    skb_free(skb);
    return 0;
}

int lo_send(struct nic *dev, const void *packet, size_t len){
    
    struct sk_buff *skb = skb_alloc(len);
    memcpy(skb_put(skb, len), packet, len);
    skb->dev = dev;
    ip_input(skb);
    return 0;
}

int lo_initialize(){
    struct nic* lo = netif_allocate();
    strcpy(lo->name, "lo");

    lo->mtu = 1500;
    lo->ip_addrs =  htonl(0x7F000001);
    lo->gateway = 0; //directly reachable afterall
    memset(lo->mac, 0, 6);

    lo->send = lo_send;
    lo->handle_rx = net_receive;
    lo->output = lo_output;
    lo->poll = NULL;
    lo->priv = NULL;

    route_add_route(lo->ip_addrs, 0xffffff00, lo->gateway, lo);
}

MODULE_INIT(lo_initialize);

