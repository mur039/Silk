#include <loop.h>
#include <syscalls.h>

static struct loop_device* device_table = NULL;
static struct loop_device* free_node_list = NULL;







int loop_control_ioctl(fs_node_t* node, unsigned long req, void* argp){

    if(req != LOOP_CTL_GET_FREE){
        return -ENOTTY;
    }

    return free_node_list->lo_number;
}


static struct fs_ops control_ops = {
    .ioctl = &loop_control_ioctl,
    .write = NULL
};



int loop_ioctl(fs_node_t* node, unsigned long req, void* argp){

    int fd = (int)argp;
    struct loop_device* ld = ((device_t*)node->device)->priv;
    
    switch(req){
        case LOOP_SET_FD: 

            if( ld->lo_inode){ //already set
                return -EEXIST;
            }
            
            if( fd < 0 || fd > MAX_OPEN_FILES){
                return -EBADF;
            }

            file_t *fdesc = &current_process->open_descriptors[fd];
            if(!fdesc->f_inode)
                return -EBADF;
            
            ld->lo_inode = (fs_node_t*)fdesc->f_inode;
            open_fs(ld->lo_inode, O_RDONLY); // just to increment refcount i suppose

            for(struct loop_device* head = free_node_list, *prev = NULL; head != NULL; head = head->next){
                if( head == ld){
                    if(prev){ //middle of the list
                        prev->next = head->next;
                    }
                    else{ //beginning of the list
                        free_node_list = head->next;
                    }
                    break;
                }
                prev = head;
            }


            return 0;
        break;

        case LOOP_CLR_FD:
            if(!ld->lo_inode){ //already free
                return -EINVAL;
            }

            ld->lo_inode->ops.close(ld->lo_inode);
            ld->lo_inode = NULL;
            ld->next = free_node_list;
            free_node_list = ld;
        break;

        
        
        case LOOP_CHANGE_FD: 
        
        break;
        case LOOP_SET_STATUS64: 
        case LOOP_GET_STATUS64: 
        default:
            return -ENOTTY;
        break;

    }

}


uint32_t loop_read(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    
    struct loop_device* ld = ((device_t*)node->device)->priv;
    if(!ld->lo_inode){
        return -ENXIO;
    }

    return read_fs(ld->lo_inode, offset, size, buffer);
}



static struct fs_ops loop_ops = {
    .ioctl = &loop_ioctl,
    .read =  &loop_read
};

void loop_install(){
    
    device_table = kcalloc(LOOP_MAX_DEVICE_COUNT, sizeof(struct loop_device));
    for(int i = 0; i < LOOP_MAX_DEVICE_COUNT; ++i){
        struct loop_device* head = &device_table[i];
        head->lo_number = i;
        head->lo_inode = NULL;
        head->lo_flags = 0;
        head->next = free_node_list;
        free_node_list = head;

    }

    //register ctrl
    device_t ctrl_dev;
    ctrl_dev.dev_type = DEVICE_CHAR;
    ctrl_dev.name = strdup("loop-control");
    ctrl_dev.unique_id = 10;
    ctrl_dev.ops = control_ops;
    dev_register(&ctrl_dev);


    for(int i = 0; i < LOOP_MAX_DEVICE_COUNT; ++i){
        struct loop_device* head = &device_table[i];
        device_t ld;
        ld.dev_type = DEVICE_BLOCK;
        ld.name = strdup("loopxxx");
        sprintf(ld.name, "loop%u", i);
        ld.unique_id = 10;
        ld.ops = loop_ops;
        ld.priv = head;
        dev_register(&ld);
    }

}