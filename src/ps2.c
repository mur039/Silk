#include <ps2.h>



ps2_controller_status_t ps2_get_status(void){
    ps2_controller_status_t ret;
    ret.raw = inb(PS2_STATUS_REG);
    return ret;
}

ps2_controller_config_t ps2_get_configuration(void){
    //flush output_buffer
    inb(PS2_DATA_PORT); //discard it

    //wait for input buffer to be empty
    while(ps2_get_status().input_buffer != 0);
    ps2_send_command(PS2_READ_BYTE_N + 0);//   outb(PS2_COMMAND_REG, PS2_READ_BYTE_N + 0); //read byte 0 from internal ram, config reg
 
    ps2_controller_config_t config;
    config.raw = inb(PS2_DATA_PORT);
    return config;
}

void ps2_set_configuration(ps2_controller_config_t config){
    ps2_send_command(PS2_SET_BYTE_N + 0);
    ps2_send_data(config.raw);
    return;
}

void ps2_send_command(unsigned char command){
    while(ps2_get_status().input_buffer != 0); //wait for input buffer to be empty
    outb(PS2_COMMAND_REG, command);
}

void ps2_send_data(unsigned char data){
    while(ps2_get_status().input_buffer != 0); //wait for input buffer to be empty
    outb(PS2_DATA_PORT, data);

}

// Wait for input buffer to be empty
void wait_input_clear() {
    while (inb(PS2_COMMAND_REG) & STATUS_INPUT_BUFFER);
}

// Wait for output buffer to be full
void wait_output_full() {
    while (!(inb(PS2_COMMAND_REG) & STATUS_OUTPUT_BUFFER));
}

