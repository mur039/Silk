#include <char.h>


read_type_t read_null(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    return 0; //eof
}

write_type_t write_null(fs_node_t * node, uint32_t offset, uint32_t size, uint8_t* buffer){
    return size;
}


read_type_t read_zero(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    for(int i = 0; i < size; ++i){
        buffer[i] = 0;
    }
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

ioctl_type_t port_ioctl(fs_node_t* node, unsigned long request, void* argp){

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
		 11 = /dev/kmsg		Writes to this come out as printk's, reads
		
        			export the buffered printk records.

*/
void install_kernel_mem_devices(){

    device_t* device;

    //mem
    //null
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("null");
    device->unique_id = 3;
    device->dev_type = DEVICE_CHAR;

    device->read = read_null;
    device->write = write_null;
    dev_register(device);
    kfree(device);

    //port
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("portio");
    device->unique_id = 4;
    device->dev_type = DEVICE_CHAR;

    device->ioctl = port_ioctl;
    dev_register(device);
    kfree(device);

    //zero
    device = kcalloc(1, sizeof(device_t));
    device->name = strdup("zero");
    device->unique_id = 5;
    device->dev_type = DEVICE_CHAR;

    device->write = write_null;
    device->read = read_zero;
    dev_register(device);
    kfree(device);
    
    //kmsg
    //woulb be interesting if implemented


}