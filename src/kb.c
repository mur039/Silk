#include <kb.h>
#include <sys.h>
#include <tty.h>
#include <vt.h>
/* KBDUS means US Keyboard Layout. This is a scancode table
*  used to layout a standard US keyboard. I have left some
*  comments in to give you an idea of what key is what, even
*  though I set it's array index to 0. You can change that to
*  whatever you want using a macro, if you wish! */
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '*', '-', 127 ,	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
  'i', '"',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '.',   0,				/* Right shift */
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



unsigned char kbdus_shifted[128] =
{
    0,  27, '!', '\'', '^', '+', '%', '&', '/', '(', ')', '=', '?', '_', 127,	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',	/* ENTER KEY */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',	/* 39 */
 'I', '`',   0,		/* Left shift */
 '\\', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', ',', '.', ':',   0,				/* Right shift */
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
    0,  0,  '>',
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};		


unsigned char kbdus_altgr[128] =
{
    0,  27, '>', 0 , '#', '$', 0 , 0, '{', '[', ']', '}', '\\', '|', 127,	/* Backspace */
  '\t',			/* Tab */
  '@', 'w', 'e', 'r',	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
  'I', '"',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '.',   0,				/* Right shift */
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
    0,  0,  '|',
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};		



/*
unsigned char kbdus[] = {
  0,    27,   '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
  '-',  '=',  0,    9,    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
  'o',  'p',  '[',  ']',  0,    0,    'a',  's',  'd',  'f',  'g',  'h',
  'j',  'k',  'l',  ';',  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
  'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',  0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
  0,    0,    0,    0,    0,    '/',  0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
  0,    0,    0,    0,    0,    0,    0,    0x2C,
};

unsigned char kbdus_shifted[] = {
  0,    27,   '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
  '_',  '+',  0,    9,    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
  'O',  'P',  '{',  '}',  0,    0,    'A',  'S',  'D',  'F',  'G',  'H',
  'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
  'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,    0x1B, 0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0x0E, 0x1C, 0,    0,    0,
  0,    0,    0,    0,    0,    '?',  0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0,
  0,    0,    0,    0,    0,    0,    0,    0x2C,
};

*/



#include <ps2.h>

static circular_buffer_t kb_buffer;
static list_t waiting_list;

void ps2_kbd_initialize(){

  kb_buffer = circular_buffer_create(32);
  waiting_list = list_create();

  ps2_controller_status_t status;
  ps2_send_command(PS2_DISABLE_FIRST_PORT);
    
  // //flush the output  
  // if(inb(PS2_COMMAND_REG) & STATUS_OUTPUT_BUFFER){
  //    inb(PS2_DATA_PORT);
  // }
    
  uint8_t tmp = inb(0x61);
  outb(0x61, tmp | 0x80);
  outb(0x61, tmp & 0x7F);
  inb(0x60);

  // // //wait until input is ready
  // ps2_send_command(PS2_CMD_TEST_FIRST_PORT);

  // // //awit for output buffer to fill
  // io_wait();
  // wait_output_full();


  // uint8_t test_result = inb(PS2_DATA_PORT);
  // if (test_result != 0x00) {
  //     uart_print(COM1, "ps/2 kbd test: failed\r\n");
  //     halt();
  //     return;
  // }


  // ps2_send_command(PS2_CMD_RESET_DEVICE);
  // io_wait();
  // wait_output_full();
  // uint8_t ack = inb(PS2_DATA_PORT);
  // if (ack != 0xFA){ // Check ACK
  //   uart_print(COM1, "ps2 reset failed\r\n");
  //   halt();
  // }

  // // Check Self-Test Pass (0xAA)
  // io_wait();
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


void keyboard_handler(struct regs *r){
    
  // ps2_kbd_handler(r);
  // return;

  unsigned char scancode = inb(0x60);
  

    /* If the top bit of the byte we read from the keyboard is
    *  set, that means that a key has just been released */
    /*
    ctrl 0x1d
    alt 0x38
    left shift 0x2a
    right shift 0x36
    */
    if(scancode == 0xe0){ //extended keycode
      return;
    }

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
    else{  //pressed

      // uart_print(COM1, "kbd_driver : scancode:%u character :%c\r\n", scancode, kbdus[scancode]);
      kbd_scancode = scancode & 0x7f;
      switch(scancode & 0x7f){
            
      
      case 0x1d: ctrl_flag = 1; break; //ctrl
      case 0x2a: case 0x36: shift_flag = 1; break; //both rshift and lshift
      case 0x38: alt_gr_flag = 1; break; //alt_gr
        
      default: //rest of the keys
        is_kbd_pressed = 1;
        kb_ch = kbdus[scancode & 0x7f];
        if(ctrl_flag) 
            kb_ch &= 0x1f;
        
        if(shift_flag){
          
            kb_ch = kbdus_shifted[scancode & 0x7f];
        }

        if(alt_gr_flag){
          kb_ch = kbdus_altgr[ scancode & 0x7F];
        }
      
      break;
      }
        
    }

    if(is_kbd_pressed){
      is_kbd_pressed = 0;
      ctrl_flag &= 1;
      shift_flag &= 1;
      alt_gr_flag &= 1;

      vt_tty_send( kb_ch | (scancode << 8) | ( (ctrl_flag  | (shift_flag << 1) | (alt_gr_flag << 2) ) << 24) );
      // fb_console_printf("[atkbd] scancode:=%x %u\n", scancode, (unsigned char)kb_ch);
      circular_buffer_putc(&kb_buffer, kb_ch);
      process_wakeup_list(&waiting_list);
    }
}



enum ps2_kbd_state{
  PS2_KB_NORMAL = 0,
  PS2_KB_EXTENDED,
  PS2_KB_EXTENDED1,
};


enum ps2_kbd_state state = PS2_KB_NORMAL;

//i should have implemented passing extra parameters to interrupt handlers...
void ps2_kbd_handler(struct regs *r){

  unsigned short scancode = inb(0x60);
  
  if(state == PS2_KB_NORMAL && scancode == PS2_SCANCODE_EXTENDED){
      state = PS2_KB_EXTENDED;
      return;
  }


  int pressed = scancode & 0x80 ? 0 : 1;
  scancode &= 0x7f;
  
  //check modifiers
  if(scancode == PS2_SCANCODE_LCONTROL){

    ctrl_flag = pressed;
    goto ps2_kbd_hand_end;
  }
  
  if(scancode == PS2_SCANCODE_LSHIFT){

    shift_flag = pressed;
    goto ps2_kbd_hand_end;
  } 
  
  if(scancode == PS2_SCANCODE_LALT){

    alt_gr_flag = pressed;
    goto ps2_kbd_hand_end;
  } 


  if(!pressed){ // if no press for non-modifier keys
    goto ps2_kbd_hand_end;
  }

  if(state == PS2_KB_EXTENDED){
    scancode = 0xff00 | (scancode & 0xff);
  }

  int kb_character = kbdus[scancode];
  
  if(ctrl_flag){
    kb_character = CTRL(kb_character);
  }

  if(shift_flag)
    kb_character = kbdus_shifted[scancode];
        

  if(alt_gr_flag)
    kb_character = kbdus_altgr[ scancode];
  
  
  vt_tty_send( kb_character | ((scancode ) << 8) | ( (ctrl_flag  | (shift_flag << 1) | (alt_gr_flag << 2) ) << 24) );

ps2_kbd_hand_end:
  state = PS2_KB_NORMAL;
  return;
}