#include <cmos.h>



static void cmos_set_register(u8 _register){
    outb( 
        CMOS_PORT_ADDRESS,
        (0 << NMI_BIT_SHIFT) | ( _register & ~(1 << NMI_BIT_SHIFT) )
        );
}


u8 cmos_read_register(u8 _register){
    cmos_set_register(_register);
    return inb(CMOS_PORT_DATA);
}