#include <kb.h>
#include <sys.h>
/* KBDUS means US Keyboard Layout. This is a scancode table
*  used to layout a standard US keyboard. I have left some
*  comments in to give you an idea of what key is what, even
*  though I set it's array index to 0. You can change that to
*  whatever you want using a macro, if you wish! */
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    224/*0*/,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    225/*0*/,	/* Left Arrow */
    0,
    226/*0*/,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    227/*0*/,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,  0,  '<',
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};		

#include <ps2.h>
void ps2_kbd_initialize(){

  ps2_controller_status_t status;
  ps2_send_command(PS2_DISABLE_FIRST_PORT);
    
  // //flush the output  
  // if(inb(PS2_COMMAND_REG) & STATUS_OUTPUT_BUFFER){
  //    inb(PS2_DATA_PORT);
  // }
    

  // // //wait until input is ready
  // ps2_send_command(PS2_CMD_TEST_FIRST_PORT);

  // //awit for output buffer to fill
  // wait_output_full();
  // uint8_t test_result = inb(PS2_DATA_PORT);
  // if (test_result != 0x00) {
  //     uart_print(COM1, "ps/2 kbd test: failed\r\n");
  //     halt();
  //     return;
  // }



  // ps2_send_command(PS2_CMD_RESET_DEVICE);
  // wait_output_full();
  // uint8_t ack = inb(PS2_DATA_PORT);
  // if (ack != 0xFA){ // Check ACK
  //   uart_print(COM1, "ps2 reset failed\r\n");
  //   halt();
  // }

  // // Check Self-Test Pass (0xAA)
  // // while (!(inb(PS2_COMMAND_REG) & STATUS_OUTPUT_BUFFER));
  // wait_output_full();
  // uint8_t self_test = inb(PS2_DATA_PORT);
  // if (self_test != 0xAA){ // Self-test failed
  //   uart_print(COM1, "Self test failes\r\n");
  //   halt();
  // }

  ps2_send_command(PS2_ENABLE_FIRST_PORT);
    
}












int ctrl_flag = 0;
int shift_flag = 0;
int alt_gr_flag = 0;
int key_modifiers;
volatile int is_kbd_pressed = 0;

char kb_ch;
uint8_t kbd_scancode;


void keyboard_handler(struct regs *r)
{
    
  if(r->err_code == 0){;}
    unsigned char scancode = inb(0x60);
    

    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    /*
    ctrl 0x1d
    alt 0x38
    left shift 0x2a
    right shift 0x36

    */
   uart_print(0x3f8, "kbd_driver: %s scancode: %u\r\n", scancode & 0x80 ? "released" : "pressed", scancode & 0x7f );

    if (scancode & 0x80) //released
    {
        if((scancode &  0x7f) == 0x1d){ //if ctrl is released
            ctrl_flag = 0;

        }else if((scancode &  0x7f) == 0x2a || (scancode &  0x7f) == 0x36 ){ //shift modifier
            shift_flag = 0;

        }
        else if( ( scancode & 0x7f) == 0x38 ){ //alt_gr
            alt_gr_flag = 0;

        }
        
    }
    else //pressed
    {
        kbd_scancode = scancode & 0x7f;
        switch(scancode & 0x7f){
            
            //ctrl
            case 0x1d: 
                ctrl_flag = 1; 
                break;

            //both rshift and lshift
            case 0x2a:
            case 0x36:
                shift_flag = 1;
                break;
            
            //alt_gr
            case 0x38:
                alt_gr_flag = 1;
                break;

            case 59: //F1
              if(alt_gr_flag){

                //well very specific thing that print the fs_tree
                vfs_tree_traverse(fs_tree);
              }
              break;
            case 75: //sol yön tuşu
            case 77: //sağ yön tuşu
              
              //switching vt's
              if(alt_gr_flag){
                int vt_increment = scancode - 76;
                

              }
              break;
            //rest of the keys
            default:
                is_kbd_pressed = 1;
                kb_ch = kbdus[scancode & 0x7f];

                // fb_console_printf("kbd_driver : %x ->  %x/%c\n", scancode & 0x7f, kb_ch, kb_ch);

                if(ctrl_flag) 
                    kb_ch &= 0x1f;
                
                if(shift_flag){
                    
                    if(kb_ch == '<'){
                        
                        kb_ch += 2;
                    }
                    else if(kb_ch >= 'a' && kb_ch <= 'z')
                    {

                        kb_ch -= 32;
                    }else if(kb_ch >= 'A' && kb_ch <= 'Z')
                    {

                        kb_ch += 32;
                    }
                    else if(kb_ch == '/'){
                        kb_ch = ':';
                    }

                }

                if(alt_gr_flag){

                    if(kb_ch == '<'){
                        kb_ch = '|';
                    }
                }

            
                break;
        }

    }
}



