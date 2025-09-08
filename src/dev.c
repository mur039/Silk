#include <dev.h>
#include <uart.h>
#include <module.h>

#define MAX_DEVICE 64
device_t devices[ MAX_DEVICE ];
static size_t lastid = 0;


void dev_init(){
	memset(devices, 0, 64*sizeof(device_t));
	lastid = 0;
	fb_console_printf("Device Manager initialized.\n");
	return;
}




int dev_register(device_t* dev)
{
	devices[lastid] = *dev;
	fb_console_printf("Registered Device %s (%u) as Device#%u\n", dev->name, dev->unique_id, lastid);
	return lastid++;
}

EXPORT_SYMBOL(dev_register);

void list_devices(){
    for(size_t i = 0; i < 64 && i < lastid; ++i){
        fb_console_printf("%u -> name:%s unique_id:%u dev_type:%s\n", 
        i,
        devices[i].name, 
        devices[i].unique_id,
        devices[i].dev_type == DEVICE_BLOCK ? "BLOCK DEVICE" : "CHARACTER DEVICE"
        );
    }
}

device_t * dev_get_by_index(size_t index){
	if(index < MAX_DEVICE && index < lastid){
		return &devices[index];
	}
	return NULL;
}

device_t * dev_get_by_name(const char* devname){
	for(size_t i = 0; i < MAX_DEVICE && i < lastid; ++i){

		if(!strcmp(devices[i].name, devname)){
			return &devices[i];
		}
	}
	return NULL;

}



fs_node_t * devfs_create() {
	fs_node_t * fnode = kmalloc(sizeof(fs_node_t));
	if(!fnode)
		error("failed to create devfs");

	memset(fnode, 0x00, sizeof(fs_node_t));
	fnode->inode = 0;
	
	strcpy(fnode->name, "devfs");
	fnode->uid = 0;
	fnode->gid = 0;
	fnode->flags   = FS_DIRECTORY;
	
	fnode->ops.readdir = (readdir_type_t)devfs_readdir;
	fnode->ops.finddir = (finddir_type_t)devfs_finddir;
	fnode->ops.ioctl   = NULL;
	return fnode;
}


uint32_t devfs_generic_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){

	device_t * dev = node->device;
	if(dev->dev_type == DEVICE_CHAR){
		return 0;
	}
	else{ //block device
		uint32_t * priv = dev->priv;
		size_t max_size = priv[0];
		uint8_t* raw_block = (void*)priv[1];

		if(offset >= max_size){
			return 0;
		}

		memcpy(buffer, &raw_block[offset], size);
		node->offset += size;
		return size;
	}
}



uint32_t devfs_generic_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){

	device_t * dev = node->device;
	if(dev->dev_type == DEVICE_CHAR){
		return 0;
	}
	else{ //block device

		uint32_t * priv = dev->priv;
		size_t max_size = priv[0];
		uint8_t* raw_block = (void*)priv[1];


		if(offset >= max_size){
			return 0;
		}

		if(offset + size >= max_size){
			size = max_size - offset;
		}

		memcpy(&raw_block[offset], buffer, size);
		node->offset += size;
		return size;
	}
}



struct fs_node* devfs_finddir(struct fs_node* node, const char *name){
    //from here name should be ../dir/
    // fb_console_printf("devfs: path:%s\n", name);

	
    //assume we have it
    device_t* dev = dev_get_by_name(name);
	
	if(!dev) // does not exists
		return NULL;

	int dev_index = (int)(dev - devices);
	if(dev_index < 0) //the fuck?
		return NULL;
    
    // fb_console_printf("found:%s\n", dev->name);
    struct fs_node * fnode = kcalloc(1, sizeof(struct fs_node));
    fnode->inode = dev_index;
	
    strcpy(fnode->name, dev->name);
    fnode->uid = 0;
	fnode->gid = 0;
	fnode->device = dev;
    fnode->flags   = dev->dev_type == DEVICE_CHAR?  FS_CHARDEVICE  : FS_BLOCKDEVICE;
	fnode->ops = dev->ops;
    return fnode;
    

    
}
 
static struct dirent * devfs_readdir(fs_node_t *node, uint32_t index) {
	// fb_console_printf("devfs_readdir: index:%u::%s\n", index, node->name);

	if(node->flags != FS_DIRECTORY){
		return NULL;
	}
	

	
	if(index < lastid){
		
		device_t dev = devices[index];
		
		struct dirent * out = kcalloc(1, sizeof(struct dirent));
		out->ino = index;
		out->type = dev.dev_type == DEVICE_CHAR ? FS_CHARDEVICE : FS_BLOCKDEVICE;
		out->off = index;
		out->reclen = 0;
		strcpy( out->name, dev.name);

		node->offset++;
		return out;
		
	}
	
	node->offset = 0;
	return NULL;
}
