    #include <char.h>
#include <syscalls.h>

uint32_t read_null(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    return 0; //eof
}

uint32_t write_null(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer){
    return size;
}


uint32_t write_full(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer){
    return -ENOMEM;
}

uint32_t read_zero(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    
    if(!is_virtaddr_mapped(buffer)){
        return -EFAULT;
    }

    memset(buffer, 0, size);
    return size;
}


#include <krand.h>
uint32_t read_random(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    
    if(!is_virtaddr_mapped(buffer)){
        return -EFAULT;
    }

    uint32_t* buff = (uint32_t*)buffer;
    uint32_t o_len = size;
    while(o_len > 4){

        *(buff++) = krand();
        o_len -= 4;
    }

    if(!o_len){
        return size;
    }

    uint32_t mask = 0;
    if(o_len == 1){
        mask = 0xff;
    }
    else if(o_len == 2){
        mask = 0xffff;
    }
    else if(o_len == 3){
        mask = 0xffffff;
    }
    else if(o_len == 4){
        mask = 0xffffffff;
    }

    *buff = (*buff & ~mask) | krand() & mask;

    return size;
}




enum port_ioctl_request{
    PORTIO_CAPABILITY = 0, //not implemented
    PORTIO_READ,
    PORTIO_WRITE,
};

typedef struct{
    uint16_t port;
    uint16_t size; // 1 2 4 8?
    uint32_t value;
} portio_request_t;

int port_ioctl(fs_node_t* node, unsigned long request, void* argp){

    portio_request_t* req = argp;
    
    if(request == PORTIO_READ){
        
        uint16_t port = req->port;
        uint16_t size = req->size;
        if(size == 1){
            req->value = inb(port);
        }
        else if(size == 2){
            req->value = inw(port);
        }
        if(size == 4){
            req->value = inl(port);
        }
        
        return 0;
    }
    else{

        uint16_t port = req->port;
        uint16_t size = req->size;
        if(size == 1){
            outb(port, req->value);
        }
        else if(size == 2){
            outw(port, req->value);
        }
        if(size == 4){
            outl(port, req->value);
        }
        return 0;
    }


}

/*
          1 = /dev/mem		Physical memory access
		  3 = /dev/null		Null device
		  4 = /dev/port		I/O port access
		  5 = /dev/zero		Null byte source
		  7 = /dev/full		Returns ENOSPC on write
		  8 = /dev/random	Nondeterministic random number gen.
		  9 = /dev/urandom	Faster, less secure random number gen.
		 10 = /dev/aio		Asynchronous I/O notification interface
		 11 = /dev/kmsg		Writes to this come out as printk's, reads export the buffered printk records.

*/
void install_kernel_mem_devices(){

    device_t* device;

    //mem
    //null
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("null");
    device->unique_id = 3;
    device->dev_type = DEVICE_CHAR;

    device->ops.read = (read_type_t)read_null;
    device->ops.write = (write_type_t)write_null;
    dev_register(device);
    kfree(device);

    //port
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("portio");
    device->unique_id = 4;
    device->dev_type = DEVICE_CHAR;

    device->ops.ioctl = (ioctl_type_t)port_ioctl;
    dev_register(device);
    kfree(device);

    //zero
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("zero");
    device->unique_id = 5;
    device->dev_type = DEVICE_CHAR;

    device->ops.write = (write_type_t)write_null;
    device->ops.read = (read_type_t)read_zero;
    dev_register(device);
    kfree(device);

    //full
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("full");
    device->unique_id = 7;
    device->dev_type = DEVICE_CHAR;

    device->ops.write = (write_type_t)write_full;
    device->ops.read = (read_type_t)read_zero;
    dev_register(device);
    kfree(device);

    //random
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("random");
    device->unique_id = 8;
    device->dev_type = DEVICE_CHAR;

    device->ops.read = (read_type_t)read_random;
    dev_register(device);
    kfree(device);
    
    //kmsg
    //woulb be interesting if implemented


}