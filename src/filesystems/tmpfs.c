#include <filesystems/tmpfs.h>
#include <g_tree.h> 




typedef struct tmpfs_device{
    
    tree_t*  tree;
    tree_node_t* self;
    uint32_t ref_count;
    uint8_t *data;
    size_t data_size;
} tmpfs_device_t;


fs_node_t * tmpfs_install(){

	fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));

    tmpfs_device_t* tmpfs = kcalloc(1, sizeof(tmpfs_device_t));
    

    //as if i maounted something here

	fnode->inode = 0;
	strcpy(fnode->name, "tmpfs");

    fnode->device = NULL;
    
    fnode->impl = 4; //?
	fnode->uid = 0;
	fnode->gid = 0;
    
	fnode->flags   = FS_DIRECTORY;
	fnode->read    = NULL;
	fnode->write   = NULL;
	fnode->open    = NULL;
	fnode->close   = NULL;
    fnode->create  = tmpfs_create;
    fnode->mkdir   = tmpfs_mkdir;
	fnode->readdir = tmpfs_readdir;
	fnode->finddir =  tmpfs_finddir;
	fnode->ioctl   = NULL;


    fnode->device = tmpfs;

    tmpfs->tree = tree_create();

	tree_set_root(tmpfs->tree, fnode);
    tmpfs->self = tmpfs->tree->root;
    tmpfs->ref_count = 0;
    tmpfs->data_size = 0;
    tmpfs->data = NULL;

	return fnode;
}


finddir_type_t tmpfs_finddir (struct fs_node* node, char *name){    

    // fb_console_printf("tmpfs_finddir: %s::%s\n", node->name, name);
    
    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return NULL;
    }


    tree_node_t* tnode = tmpfs->self;
    fs_node_t* fnode;

    for(listnode_t* cnode = tnode->children->head; cnode ; cnode = cnode->next){
        tree_node_t* ctnode = cnode->val;
        fnode = ctnode->value;
            
        if(!strcmp(fnode->name, name)){
            // fb_console_printf("found: %s, \n", fnode->name);                
            return fnode;
        }
    } 

    return NULL;
}


struct dirent * tmpfs_readdir(fs_node_t *node, uint32_t index) {
    // fb_console_printf("tmpfs_readdir: index:%u:%s\n", index, node->name);
    

    if(node->flags != FS_DIRECTORY){
		return NULL;
	}	

    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return NULL;
    }

    tree_node_t* tnode = tmpfs->self;
    
    if(index < tnode->children->size){
        fs_node_t* fnode;

        listnode_t* cnode = tnode->children->head;

        for(uint32_t i = 0; i < index; ++i){
            cnode = cnode->next;
        }

        tree_node_t* ctnode = cnode->val;
        fnode = ctnode->value;
        

        struct dirent* out = kcalloc(1, sizeof(struct dirent));
        out->ino = index;
        out->type = fnode->flags;
        out->off = index;
        out->reclen = 0;
        strcpy(out->name, fnode->name);

        node->offset++;
        return out;


    }
    
    node->offset = 0;
    return NULL;
}

int tmpfs_unlink(struct fs_node *node , char *name);

create_type_t tmpfs_create(fs_node_t* node, char* name, uint16_t permissions){
    fb_console_printf("tmpfs_create: name:%s permissions:%x\n", name, permissions );
    
    fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));
    tmpfs_device_t* tmpfs = kcalloc(1, sizeof(struct tmpfs_internal_struct));
    *tmpfs = *(tmpfs_device_t*)node->device;

    
	fnode->inode = 0;
	strcpy(fnode->name, name);
    
    fnode->impl = 4; //?
	fnode->uid = 0;
	fnode->gid = 0;

	fnode->flags   = FS_FILE;
	fnode->read    = tmpfs_read;
	fnode->write   = tmpfs_write;
    fnode->unlink  = tmpfs_unlink;
	fnode->open    = NULL;
	fnode->close   = NULL;
	fnode->readdir = NULL;
	fnode->finddir = NULL;
	fnode->ioctl   = NULL;

    fnode->device = tmpfs;
    tmpfs->data = NULL; //put a page here
    tmpfs->data_size = 0; //put a page here
    tmpfs->ref_count = 0; //put a page here

    tmpfs->self = tree_node_insert_child(tmpfs->tree, tmpfs->self, fnode);

	return;
}


mkdir_type_t tmpfs_mkdir(fs_node_t* node, char* name, uint16_t permissions){

    fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));
    tmpfs_device_t* tmpfs = kcalloc(1, sizeof(tmpfs_device_t));
    *tmpfs = *(tmpfs_device_t*)node->device;

    
	fnode->inode = 0;
	strcpy(fnode->name, name);
    
    fnode->impl = 4; //?
	fnode->uid = 0;
	fnode->gid = 0;

	fnode->flags   = FS_DIRECTORY;
	fnode->read    = NULL;
	fnode->write   = NULL;
	fnode->open    = NULL;
	fnode->close   = NULL;

    fnode->create  = tmpfs_create;
    fnode->mkdir   = tmpfs_mkdir;
	fnode->readdir = tmpfs_readdir;
	fnode->finddir = tmpfs_finddir;
	fnode->ioctl   = NULL;

    fnode->device = tmpfs;
    tmpfs->self = tree_node_insert_child(tmpfs->tree, tmpfs->self, fnode);

	return;
}



write_type_t tmpfs_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){

    if( node->flags == FS_DIRECTORY || node->flags == FS_MOUNTPOINT ){
        return 0;
    }

    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return 0;
    }

    //need to do some allocations
    if(!tmpfs->data){ //no allocations occured so
        tmpfs->data = kmalloc(4*1024); //4kB buffer
        if(!tmpfs->data)
            error("failed allocate private storage");
        tmpfs->data_size = 4*1024;
    }

    if(offset + size > tmpfs->data_size){
        tmpfs->data = krealloc(tmpfs->data, tmpfs->data_size + 4096);
        tmpfs->data_size += 4096;
    }

    //successfull
    uint8_t* raw_block = tmpfs->data;
    memcpy(&raw_block[offset], buffer,  size);
    node->offset  += size;
    node->length += size;
	return size;
}




read_type_t tmpfs_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){

    if( node->flags == FS_DIRECTORY || node->flags == FS_MOUNTPOINT ){
        return 0;
    }

    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return NULL;
    }


    uint8_t* raw_block = tmpfs->data;

    size_t left_size = node->length - offset;

    if(left_size == 0) //eof
        return 0;
    
    if(size > left_size){
        size = left_size;
    }

    //still some data here
    memcpy(buffer, &raw_block[offset], size);    
    node->offset += size;
	return size;
}

void tmpfs_close_fs(fs_node_t *node){

    return;
    if(node->flags == FS_DIRECTORY || node->flags == FS_MOUNTPOINT)
        return;


    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return;
    }

    node->offset = 0;
    if(tmpfs->ref_count) 
        tmpfs->ref_count--;

}

int tmpfs_unlink(struct fs_node *_fnode , char *name){
    

     if( _fnode->flags == FS_DIRECTORY || _fnode->flags == FS_MOUNTPOINT ){
        return 0;
    }

    tmpfs_device_t* tmpfs = _fnode->device;
    if(!tmpfs){        
        return -1;
    }

    fb_console_printf("node : %s - %s\n", _fnode->name, name);

    //remove from the parent
    tree_node_t* self =  tmpfs->self;
    for(listnode_t* node = self->parent->children->head; node; node = node->next){
        tree_node_t* tnode = node->val;
        fs_node_t* fnode = tnode->value;

        if( fnode == _fnode){
            listnode_t* ret = list_remove(self->parent->children, node);
            kfree(ret->val);
            kfree(ret);
            kfree(tmpfs->data);
            kfree(tmpfs);
            kfree(_fnode);
            return 0;
        }
    }
    
    return -1;

}