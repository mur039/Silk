#include <network/skb.h>
#include <pmm.h>

void *skb_pull(struct sk_buff *skb, size_t len) {
    if (len > skb->len) return NULL; // too much pull
    skb->data += len;
    skb->len  -= len;
    return skb->data;
}

void *skb_push(struct sk_buff *skb, size_t len) {
    skb->data -= len;
    skb->len  += len;
    if (skb->data < skb->head) {
        // underflow guard
        return NULL;
    }
    return skb->data;
}

void *skb_put(struct sk_buff *skb, size_t len) {
    void *tmp = skb->tail;
    skb->tail += len;
    skb->len  += len;
    if (skb->tail > skb->end) {
        // overflow guard (in real kernel you'd BUG or drop)
        return NULL;
    }
    return tmp;
}

void skb_reserve(struct sk_buff *skb, size_t len) {
    skb->data += len;
    skb->tail += len;
}

struct sk_buff *skb_alloc(size_t size) {
    struct sk_buff *skb = kmalloc(sizeof(*skb));
    if (!skb) return NULL;

    skb->head = kmalloc(size);
    if (!skb->head) {
        kfree(skb);
        return NULL;
    }

    skb->data = skb->head;
    skb->tail = skb->head;
    skb->end  = skb->head + size;
    skb->len  = 0;
    skb->dev  = NULL;
    skb->next = NULL;

    return skb;
}

void skb_free(struct sk_buff *skb) {
    if (!skb) return;
    kfree(skb->head);
    kfree(skb);
}

void skb_trim(struct sk_buff *skb, size_t new_len) {
    if (new_len > skb->len) {
       // Can't extend beyond current buffer
        new_len = skb->len;
    }
    skb->len = new_len;
}
