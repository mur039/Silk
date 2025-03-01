#include <ps2_mouse.h>
#include <circular_buffer.h>

void ps2_mouse_send_command(unsigned char command){
    ps2_send_command(PS2_WRITE_BYTE_SECOND_PORT_INPUT);
    ps2_send_data(command);
}




circular_buffer_t ps2_mouse_cb;

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
    ps2_mouse_cb = circular_buffer_create(3 * 10);

}



volatile int is_available = 0;
int lock = 0;
ps2_mouse_generic_package_t ps2_mouse_irq_package;
void ps2_mouse_handler(struct regs *r){
    (void)r;
        ps2_mouse_generic_package_t in;
        u8 * phead = (u8*)&in;
        for(int i = 0; i < 3; ++i){
            
            phead[i] = inb(PS2_DATA_PORT);
        }


    if(!lock){
        ps2_mouse_irq_package = in;
        lock = 1;
    }
    
    
    // fb_console_printf("PkcsS/2 mouse  (dx, dy, button): %x %x %x\r", ps2_mouse_irq_package.x_axis_move& 0xff, ps2_mouse_irq_package.y_axis_move & 0xff, ps2_mouse_irq_package.bits.raw);
    return;
}

 //data is 3 byte and is little endian encoded so sign bit can be used if call is succesfull
int32_t ps2_mouse_read(){
    unsigned int max_tries = 1000;
    while(max_tries--){

        if(lock){
                        
            lock = 0;
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
    if(data != -1){
        char * p8 = (u8 *)&data;
        for(int i = 0; i < 3; ++i){
            buffer[i] = p8[i]; 
        }
        return 3;
    }

    return 0;


}
