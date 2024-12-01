#ifndef __PS_2_H__
#define __PS_2_H__

#include <stdint.h>
#include <sys.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <uart.h>
#include <pmm.h>
#include <str.h>
#include <fb.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_REG 0x64 //on read
#define PS2_COMMAND_REG 0x64 //on write

/*COMMAND LIST*/
#define PS2_READ_BYTE_N 0x20 // command ) 0x20 + N
#define PS2_SET_BYTE_N 0x60 // command ) 0x20 + N

#define PS2_ENABLE_SECOND_PORT 0xA8
#define PS2_DISABLE_SECOND_PORT 0xA7

#define PS2_WRITE_BYTE_SECOND_PORT_INPUT 0xD4

union ps2_controller_status {
    struct {
        unsigned char output_buffer:1;
        unsigned char input_buffer:1;
        unsigned char system_flag:1; //cleared on reset and set by firmware is POST is passed
        unsigned char command_data:1; //0 data written to the device, 1 data written to the ps/2 controller
        unsigned char unknown0:1;//chip specific
        unsigned char unknown1:1;//chip specific
        unsigned char time_out_error:1;
        unsigned char parity_error:1;
    };
    unsigned char raw;
};

typedef union ps2_controller_status ps2_controller_status_t;


union ps2_controller_config{
    struct{
        unsigned char first_port_interrupt:1;
        unsigned char second_port_interrupt:1;
        unsigned char system_flag:1; //cleared on reset and set by firmware is POST is passed
        unsigned char zero:1; //should be zero
        unsigned char first_port_clock:1; //1:disabled 0:enabled
        unsigned char second_port_clock:1; //1:disabled 0:enabled
        unsigned char first_port_translation:1;
        unsigned char zero2:1; //must be zero
    };
    unsigned char raw;
};

typedef union ps2_controller_config ps2_controller_config_t;




ps2_controller_status_t ps2_get_status(void);
ps2_controller_config_t ps2_get_configuration(void);
void ps2_set_configuration(ps2_controller_config_t config);
void ps2_send_command(unsigned char command);
void ps2_send_data(unsigned char data);

#endif