#include <module.h>
#include <dev.h>

typedef  unsigned long long uint64_t;
typedef void (*timer_func_t)(void *user_data);
extern int kprintf(const char* fmt, ...);
extern int timer_register(uint64_t delay_ticks, uint64_t interval_ticks, timer_func_t cb, void *arg);

timer_func_t func(void* d){
    kprintf("My life is in your hand i foe and fie\n");
    return 0;
}


read_type_t hello_read(fs_node_t* node , uint32_t offset, uint32_t size, uint8_t* buffer){
    char* data = "Hello World!\n";

    if(offset >= 13){
       return 0;
    }

    uint32_t remaining = 13 - offset;
    if(size > remaining){
        size = remaining;
    }

    for(uint32_t i = 0; i < size; ++i){
        buffer[i] = data[offset + i];
    }
    return size;
};

device_t mydev = {
    .name = "hello",
    .read =  hello_read,
    .dev_type = DEVICE_CHAR,
    

    .unique_id = 255

};

static int module_initialize(void){
    kprintf("[testmodule]: I'm lord of all darkness, i'm queen of the night\n");
    dev_register(&mydev);
    return 0;
}

MODULE_INIT(module_initialize);

void module_finish(){
    kprintf("Now do the march of the black queen\n");
    return;
}

MODULE_EXIT(module_finish);