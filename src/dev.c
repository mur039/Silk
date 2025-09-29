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




// fs_node_t* devfs_mount(fs_node_t* nodev, const char* options){
// 	fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));
// 	if(!fnode)
// 		error("failed to create devfs");

	
// 	fnode->inode = 1;
	
// 	strcpy(fnode->name, "devfs");
// 	fnode->uid = 0;
// 	fnode->gid = 0;
// 	fnode->flags   = FS_DIRECTORY;
	
// 	fnode->ops.readdir = (readdir_type_t)devfs_readdir;
// 	fnode->ops.finddir = (finddir_type_t)devfs_finddir;
// 	fnode->ops.ioctl   = NULL;
// }

// int devfs_probe(fs_node_t* nodev){
// 	if(nodev){
// 		return 0;
// 	}

// 	return 1;
// }

// struct filesystem devfs_fs = {
// 	.fs_name = "dev",
// 	.probe = &devfs_probe,
// 	.mount = &devfs_mount,
// };

// static int devfs_init(){
// 	// devfs_root = tree_create();
// 	return fs_register_filesystem(&devfs_fs);
// }


// MODULE_INIT(devfs_init);