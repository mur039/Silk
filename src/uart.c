#include <uart.h>
#include <sys.h>
#include <str.h>
#include <queue.h>



int init_uart_port(int port)
{
    //gotta do somthing about it , interrupts doesn't trigger
    outb(port + 1, 0x00);    // Disable all interrupts
    outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00);    //                  (hi byte)
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(port + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
 
   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(port + 0) != 0xAE) {
      return 1;
   }
 
   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(port + 4, 0x0F);
   outb(port + 1, 0b0001);//wnable daata available interrupt
   return 0;
}


int is_transmit_empty(int port){
    return inb(port + 5) & 0x20;
}
void uart_write(int port, const void * src, uint32_t size, uint32_t nmemb){
    while(is_transmit_empty(port) == 0);
    for(uint32_t i = 0; i < nmemb; ++i){
        uint8_t byte = 0;
        byte = *((uint8_t *)src + (i*size));
        outb(port, byte);
    }
}

int serial_received(int port){
   return inb(port + 5) & 1;
}
char serial_read(int port){
    while (serial_received(port) == 0);
    return inb(port);
}

static int dest_port = 0;
static char uart_gprintf_wrapper(char c){
    uart_write(dest_port, &c, 1, 1);
    return c;
}

void uart_print(int port, const char * source, ...){
    va_list arg;
    va_start(arg, source);
    dest_port = port;
    va_gprintf(
        uart_gprintf_wrapper,
        source,
        arg
    );
    // for(int i = 0; source[i] != '\0'; ++i){
    //     uart_write(port, &source[i], 1, 1);
    // }
    return;
}






typedef  struct uart_console_priv_dat{
    queue_t work_queue;
    circular_buffer_t internal_circular_buffer;
    uint8_t refcount;

} uart_console_priv_dat_t;

extern volatile int is_received_char;
extern volatile char ch;

#include <process.h>
typedef struct uart_work_desc {
    pcb_t* req_proc;
    size_t byte_size;
} uart_work_desc_t;


device_t* com1_dev = NULL;

void uart_handler(struct regs *r){
    (void)r;
    
    char c = serial_read(0x3f8);
    
    if(com1_dev){
        uart_console_priv_dat_t* priv = com1_dev->priv;
        if(priv->refcount){
            
            circular_buffer_putc(&priv->internal_circular_buffer, c);

            int size = circular_buffer_avaliable(&priv->internal_circular_buffer);
            uart_work_desc_t* pending_job = queue_dequeue_item(&priv->work_queue);
        

            if(pending_job){ //check if there's a pending job

                if(size >= pending_job->byte_size){
                    //unblock the process
                    // pending_job->req_proc->regs.eip -= 2; //wind it back to the int 0x80 call
                    pending_job->req_proc->state = TASK_RUNNING;

                }
                else{
                //if not push job back to the queue
                queue_enqueue_item(&priv->work_queue, pending_job);
                }
            }

        }
    }

    is_received_char = 1;
    ch = c;

    return;
}



static int port_addr_index(int port){
    if(port == COM1){
        return 0;
    }
    return -1;
}




device_t* create_uart_device(int port){

    if( init_uart_port(port) ){ //faulty 
        return NULL;
    }


    int pindex = port_addr_index(port);
    device_t* dev = kcalloc(1, sizeof(device_t));
    if(!dev)
        error("failed allocate for dev");

    dev->name = kmalloc(5);
    if(!dev->name)
        error("failed allocate for dev->name");

    sprintf(dev->name, "ttyS%u", pindex);

    dev->write = uart_console_write;
    dev->read = uart_console_read;
    dev->open = uart_console_open;
    dev->close = uart_console_close;
    dev->dev_type = DEVICE_CHAR;
    dev->unique_id = port;

    dev->unique_id = pindex;
    
    uart_console_priv_dat_t* priv = kmalloc(sizeof(uart_console_priv_dat_t));
    if(!priv)
        error("failed allocate for priv");
    
    priv->work_queue = queue_create(1);
    priv->internal_circular_buffer  = circular_buffer_create(256);
    priv->refcount = 0;
    dev->priv = priv;
    
    com1_dev = dev;
    
    return dev;    
}



open_type_t uart_console_open(fs_node_t* node, uint8_t read, uint8_t write){
    

    device_t* dev = node->device;
    uart_console_priv_dat_t* priv = dev->priv;

    priv->refcount++;
    return;
}


close_type_t uart_console_close(fs_node_t* node){
    
    device_t* dev = node->device;
    uart_console_priv_dat_t* priv = dev->priv;
     
    priv->refcount--;
    return;
}



write_type_t uart_console_write(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer){
    (void)node;
    (void)offset;

    uart_write(COM1, buffer, 1, size);
    return size;
}   


read_type_t uart_console_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    (void)node;
    (void)offset;
    // (void)size;

    device_t* dev = node->device;
    uart_console_priv_dat_t* priv = dev->priv;

    int availble_char = circular_buffer_avaliable( &priv->internal_circular_buffer );
    

    if( size <= availble_char){ //enough bytes in buffer
        circular_buffer_read(&priv->internal_circular_buffer, buffer, 1, size);
        return size;
    }

    //should block but how?
    uart_work_desc_t* job = kcalloc(1, sizeof(uart_work_desc_t));
    
    job->req_proc = current_process;
    job->byte_size = size;
    queue_enqueue_item(&priv->work_queue, job);
    return -1;
}


