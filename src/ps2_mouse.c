#include <ps2_mouse.h>

void ps2_mouse_send_command(unsigned char command){
    ps2_send_command(PS2_WRITE_BYTE_SECOND_PORT_INPUT);
    ps2_send_data(command);
}


void ps2_mouse_initialize(){

    //enable its interrupts
    ps2_controller_config_t conf = ps2_get_configuration();
    conf.second_port_clock = 0; //enable clock
    conf.second_port_interrupt = 1; //enable interrupt
    ps2_set_configuration(conf);

    ps2_send_command(PS2_ENABLE_SECOND_PORT);
    ps2_mouse_send_command(PS2_MOUSE_DEFAULTS);
    ps2_mouse_send_command(PS2_MOUSE_ENABLE_REPORTING);
    //mouse should return ack so read and discard it
    inb(PS2_DATA_PORT);

}



volatile int is_available = 0;
ps2_mouse_generic_package_t ps2_mouse_irq_package;
void ps2_mouse_handler(struct regs *r){
    (void)r;
    
    if(!is_available) is_available = 1;
    u8 * phead = &ps2_mouse_irq_package;
    for(int i = 0; i < 3; ++i){
       phead[i] = inb(PS2_DATA_PORT);
    } 

    // fb_console_printf("PS/2 mouse  (dx, dy): %x %x\n", pack.x_axis_move& 0xff, pack.y_axis_move & 0xff);
    return;
}

 //data is 3 byte and is little endian encoded so sign bit can be used if call is succesfull
int32_t ps2_mouse_read(){
    unsigned int max_tries = 1000;
    while(max_tries--){
        if(is_available){
            is_available = 0;
            // fb_console_printf("PS/2 mouse  (dx, dy): %x %x\n", ps2_mouse_irq_package.x_axis_move& 0xff, ps2_mouse_irq_package.y_axis_move & 0xff);
            return (int32_t)ps2_mouse_irq_package.raw;
        }
    }

    return -1;

}

uint8_t ps2_mouse_fs_read(uint8_t * buffer, uint32_t offset, uint32_t len, void * dev){
    offset;
    len;
    dev;

    int32_t data = ps2_mouse_read();
    char * p8 = &data;
    if(data != -1){
        for(int i = 0; i < 3; ++i){
            buffer[i] = p8[i]; 
        }
        return 3;
    }

    return 0;


}
