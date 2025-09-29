#include <ps2_mouse.h>
#include <circular_buffer.h>
#include <process.h>
#include <syscalls.h>
void ps2_mouse_send_command(unsigned char command){
    ps2_send_command(PS2_WRITE_BYTE_SECOND_PORT_INPUT);
    ps2_send_data(command);
}



//buffering and reference counting
int refcount = 0;
circular_buffer_t psaux_ib;
list_t wait_queue;



void psaux_open(fs_node_t* n, int read, int write){
    
    if(refcount == 0){
        //flush the psaux_ib
        psaux_ib.read = 0;
        psaux_ib.write = 0;
    }
    refcount++;
    return;
}

void psaux_close(fs_node_t* n){
    if(refcount > 0){
        refcount--;
    }
    return;
}



uint32_t psaux_read(fs_node_t* n, uint32_t off, uint32_t size, uint8_t *buffer){

    circular_buffer_t *b =  ((device_t*)n->device)->priv;
    size_t  remaining = circular_buffer_avaliable(b);

    if(remaining < size){ //too much
        
        if(!list_find_by_val(&wait_queue, current_process)){
            list_insert_end(&wait_queue, current_process);
        }
        return -1;
    }

    if(!is_virtaddr_mapped(buffer)){
        return -EFAULT;
    }


    circular_buffer_read(b, buffer, 1, size);
    return size;
}


short psaux_poll(struct fs_node *fn, struct poll_table* pt){

    
    short mask = 0;
    poll_wait(fn, &wait_queue, pt);
    
    circular_buffer_t *b =  ((device_t*)fn->device)->priv;
    size_t avaliable = circular_buffer_avaliable(b);
    if( avaliable >= 3){
        mask |= POLLIN;
    }

    return mask;
}


struct fs_ops psaux_ops = {
    .open = (open_type_t)&psaux_open,
    .close = (close_type_t)&psaux_close,
    .read = &psaux_read,
    .poll = &psaux_poll

};

uint8_t pkt[3];
int idx = 0;
void ps2_mouse_handler(struct regs *r){
    (void)r; //unused
    
    uint8_t in = inb(PS2_DATA_PORT);
    
    if(refcount == 0){ //no one listening
        return;
    }

    pkt[idx++] = in;
    if(idx == 3){
        idx = 0;
        circular_buffer_write(&psaux_ib, pkt, 1, 3);
        process_wakeup_list(&wait_queue);
    }
    return;
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
    
    refcount = 0;
    psaux_ib = circular_buffer_create(3 * 10); //10 entries
    wait_queue = list_create();

    device_t psaux;
    psaux.dev_type = DEVICE_CHAR;
    psaux.name = "psaux";
    psaux.priv = &psaux_ib;
    psaux.ops = psaux_ops;
    dev_register(&psaux);

    irq_install_handler(   PS2_MOUSE_IRQ, ps2_mouse_handler);
}


