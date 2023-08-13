#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <system.h>
#include <vga.h>
#include <gdt.h>


int main(){
    gdt_install();
    char * string = "you never had i heart."; //len == 22
    initVGATerm();
    puts(string);
    scroll();
    //memsetw(((unsigned short *)0xb8000), (( 15 << 4| 0 ) << 8 ) | 'A', 1);
    for(;;);

    return 0;
}