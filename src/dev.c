#include <dev.h>

#define MAX_DEVICE 64
device_t *devices = 0;
int lastid = 0;


void dev_init(){
	devices = (device_t*)kmalloc(MAX_DEVICE * sizeof(device_t)); //max 64 devices it seems
	memset(devices, 0, 64*sizeof(device_t));
	lastid = 0;
	fb_console_printf("Device Manager initialized.\n");
	return;
}


int dev_register(device_t* dev)
{
	devices[lastid] = *dev;
	fb_console_printf("Registered Device %s (%u) as Device#%u\n", dev->name, dev->unique_id, lastid);
	lastid++;
	return lastid-1;
}

void list_devices(){
    for(int i = 0; i < 64 && i < lastid; ++i){
        fb_console_printf("%u -> name:%s unique_id:%u dev_type:%s\n", 
        i,
        devices[i].name, 
        devices[i].unique_id,
        devices[i].dev_type == DEVICE_BLOCK ? "BLOCK DEVICE" : "CHARACTER DEVICE"
        );
    }
}

device_t * dev_get_by_index(int index){
	if(index < MAX_DEVICE && index < lastid){
		return &devices[index];
	}
	return NULL;
}