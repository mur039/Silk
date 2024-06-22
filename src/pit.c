#include <pit.h>

void disable_pit(){
    outb(0x43, 0x34);
    return;
}