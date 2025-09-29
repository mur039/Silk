#include <module.h>
#include <filesystems/fs.h>
#include <filesystems/vfs.h>
#include <dev.h>
#include <process.h>
#include <circular_buffer.h>
#include <syscalls.h>
extern int kprintf(const char* fmt, ...);
unsigned long fuse_devid = 0;

enum fuse_opcode {

    FUSE_LOOKUP = 1,
    FUSE_OPEN = 14,
    FUSE_READ = 15,
    FUSE_READDIR = 28,
};


struct fuse_mountpoint{
    fs_node_t* channel;
    list_t request_queue;
    list_t reply_queue;

    list_t reply_waitqueue;
    list_t request_waitqueue;
};

struct fuse_reply{
    unsigned long len;
    signed long error;       // <0 means errno
    unsigned long long unique;     // must match request
    // payload follows: for LOOKUP, node attributes; for READ, file data
};


struct fuse_req {
    unsigned long opcode;   // e.g. FUSE_LOOKUP, FUSE_READ
    unsigned long long unique;   // request ID
    unsigned long long nodeid;   // inode id
    unsigned long long offset;   // read offset, etc.
    unsigned long len;      // size of payload
    // payload follows
};



uint32_t fuse_conn_read(struct fs_node* fnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    struct fuse_mountpoint* mnt = fnode->device;

    listnode_t* node = list_pop_end(&mnt->request_queue);
    if(!node){
        list_insert_end(&mnt->request_waitqueue, current_process);
        return -1;
    }
    struct fuse_req* req = node->val;
    if(size < sizeof(struct fuse_req) + req->len){
        return -ENXIO;
    }
    memcpy(buffer, req, sizeof(struct fuse_req) + req->len);
    
    kfree(node);
    kfree(req);
    return sizeof(struct fuse_req) + req->len;
}

uint32_t fuse_conn_write(struct fs_node* fnode, uint32_t offset, uint32_t size, uint8_t* buffer){
    struct fuse_mountpoint* mnt = fnode->device;

    struct fuse_reply* reply = kcalloc(1, sizeof(struct fuse_reply));
    memcpy(reply, buffer, sizeof(*reply));
    list_insert_end(&mnt->reply_queue, reply);
    process_wakeup_list(&mnt->reply_waitqueue);
    return size;
}


struct fs_ops fuse_conn = {
    .read = fuse_conn_read,
    .write = fuse_conn_write
};



struct dirent* fuse_mount_readdir(struct fs_node* fnode , uint32_t index){
    struct fuse_mountpoint* mnt = fnode->device;

    struct fuse_req* req = kcalloc(1, sizeof(struct fuse_req));
    req->opcode = FUSE_READDIR;
    req->nodeid = 1;
    req->unique = 1;
    req->offset = index;
    req->len = 0;
    
    
    list_insert_end(&mnt->request_queue, req);//query request
    process_wakeup_list(&mnt->request_waitqueue);

    //then wait for reply from userspace
    sleep_on(&mnt->reply_waitqueue);

    listnode_t* node = list_pop_end(&mnt->reply_queue);
    char* reply = node->val;
    if(*reply == '!'){
        fnode->offset = 0;
        return NULL;
    }
    
    struct dirent* out = kcalloc(1, sizeof(struct dirent));

    out->ino = 1;
    out->off = index;
    out->reclen= 0;
    switch(reply[0]){
        case 'r':
            out->type = FS_FILE;
        break;
        case 'b':
            out->type = FS_BLOCKDEVICE;
        break;
        case 'c':
            out->type = FS_CHARDEVICE;
        break;
        case 'd':
            out->type = FS_DIRECTORY;
        break;
    }

    strcpy(out->name, reply + 1);

    kfree(node);
    kfree(reply);
    fnode->offset++;
    return out;
}

fs_node_t* fuse_mount_finddir(struct fs_node* fnode , char* name){
     
    struct fuse_mountpoint* mnt = fnode->device;
    size_t package_length = strlen(name);
    struct fuse_req* req = kcalloc(1, sizeof(struct fuse_req) + package_length);
    req->opcode = FUSE_LOOKUP;
    req->nodeid = 1;
    req->unique = 1;
    req->offset = 0;
    req->len = package_length;
    memcpy(&req[1], name, package_length);
    
    
    list_insert_end(&mnt->request_queue, req);//query request
    process_wakeup_list(&mnt->request_waitqueue);

    //then wait for reply from userspace
    sleep_on(&mnt->reply_waitqueue);

    listnode_t* node = list_pop_end(&mnt->reply_queue);
    char* reply = node->val;
    if(*reply == '!'){
        return NULL;
    }
    
    struct fs_node* out = kcalloc(1, sizeof(struct fs_node));
    strcpy(out->name, reply + 1);
    switch(reply[0]){
        case 'r':
            out->flags = FS_FILE;
        break;
        case 'b':
            out->flags = FS_BLOCKDEVICE;
        break;
        case 'c':
            out->flags = FS_CHARDEVICE;
        break;
        case 'd':
            out->flags = FS_DIRECTORY;
            out->ops.readdir = (readdir_type_t)fuse_mount_readdir;
            out->ops.finddir = (finddir_type_t)fuse_mount_finddir;
        break;
    }


    return out;
}





struct fs_ops fuse_mount = {
    .readdir = (readdir_type_t)fuse_mount_readdir,
    .finddir = (finddir_type_t)fuse_mount_finddir
};


fs_node_t* fuse_fs_mount(fs_node_t* nodev, const char* option){

    //look for fd= in
    int fd = 0;
    int state = 0;

    for(const char *c = option; *c; c++){

        if(state == 0 && *c == 'f'){
            state = 1;
            continue;
        }

        if(state == 1 && *c == 'd'){
            state = 2;
            continue;
        }

        if(state == 2 && *c == '='){
            state = 3;
            continue;
        }

        if(state == 3){
            fd *= 10;
            fd += *c - '0';
            break;
        }
    }

    
    if(state != 3){
        return NULL;
    }

    file_t* channel =  process_get_file(process_get_current_process(), fd);
    if(!channel){
        return NULL;
    }
    
    //now check whethet the filedescriptor
    //points to the /dev/fuse device
    fs_node_t* fnode = (fs_node_t*)channel->f_inode;
    device_t* dev = fnode->device;
    if( strcmp("fuse", dev->name) && fnode->inode != fuse_devid){
        return NULL;
    }

    struct fuse_mountpoint* priv = kcalloc(1, sizeof(struct fuse_mountpoint));
    
    //create mountpoint node
    fnode->refcount++;
    priv->channel = fnode;

    //manipulate fnode that
    fnode->device = priv;
    fnode->ops = fuse_conn;

    struct fs_node* mount = kcalloc(1, sizeof(struct fs_node));
    mount->inode = 1;
    mount->refcount = 1;
    strcpy(mount->name, "fuse");
    mount->flags = FS_DIRECTORY;
    mount->device = priv;
    mount->ops = fuse_mount;
    

    return mount;
}

int fuse_fs_probe(fs_node_t* fnode){
    return 1;
}

filesystem_t fusefs_driver = {
    .fs_name = "fuse",
    .probe = fuse_fs_probe,
    .mount = fuse_fs_mount
};

device_t fusedevice ={
    .dev_type = DEVICE_CHAR,
    .name = "fuse",
};

int fuse_init(){
    fuse_devid = dev_register(&fusedevice);
    fs_register_filesystem(&fusefs_driver);
    return 1;
}

MODULE_INIT(fuse_init);