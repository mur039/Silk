#include <filesystems/tmpfs.h>
#include <g_tree.h> 

typedef struct tmpfs_device{
    
    tree_t*  tree;
    tree_node_t* self;
    uint32_t ref_count;

    //maybe turn it into a list of pages? like fat but easier?
    char *name;
	int    type;
	int    mask;
	uint32_t    uid;
	uint32_t    gid;
	unsigned int atime;
	unsigned int mtime;
	unsigned int ctime;

    //only for the files
    list_t data_page_list;
    size_t data_size;
    

} tmpfs_device_t;


fs_node_t * tmpfs_install(){

	fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));

	fnode->inode = 0;
	strcpy(fnode->name, "tmpfs");

    fnode->device = NULL;
    
    fnode->impl = 4; //?
	fnode->uid = 0;
	fnode->gid = 0;
    
	fnode->flags   = FS_DIRECTORY;

	// fnode->open    = tmpfs_open;
	// fnode->close   = tmpfs_close;

    fnode->ops.create  = (create_type_t)tmpfs_create;
    fnode->ops.mkdir   = (mkdir_type_t)tmpfs_mkdir;
    fnode->ops.unlink  = (unlink_type_t)tmpfs_unlink;
	fnode->ops.readdir = (readdir_type_t)tmpfs_readdir;
	fnode->ops.finddir = (finddir_type_t)tmpfs_finddir;


    tmpfs_device_t* tmpfs = kcalloc(1, sizeof(tmpfs_device_t));
    fnode->device = tmpfs;

    tmpfs->name = "/";
    tmpfs->type = FS_DIRECTORY;
    tmpfs->mask = 0777;

    tmpfs->tree = tree_create();

	tree_set_root(tmpfs->tree, tmpfs);
    tmpfs->self = tmpfs->tree->root;

    tmpfs->ref_count = 0;
    tmpfs->data_size = 0;
    tmpfs->data_page_list = list_create();

	return fnode;
}


struct fs_node* tmpfs_finddir (struct fs_node* node, char *name){    

    //fb_console_printf("tmpfs_finddir: %s::%s\n", node->name, name);
    
    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return NULL;
    }


    tree_node_t* tnode = tmpfs->self;
    

    for(listnode_t* cnode = tnode->children->head; cnode ; cnode = cnode->next){
        tree_node_t* ctnode = cnode->val;
        tmpfs_device_t* _tmpfs = ctnode->value;
            
        if(!strcmp(_tmpfs->name, name)){
            fs_node_t* fnode = kcalloc(1, sizeof(fs_node_t));
            fnode->inode = 5; //????
            strcpy(fnode->name, _tmpfs->name);
            fnode->uid = 0;
            fnode->gid = 0;
            fnode->impl = 9; //?
            fnode->device = _tmpfs;

            
            fnode->ops.open = tmpfs_open;
            fnode->ops.close = (close_type_t)tmpfs_close;

            if(_tmpfs->type == FS_FILE){
                
                fnode->flags = FS_FILE;
                fnode->length = _tmpfs->data_size;
            }
            else{
                
                fnode->flags = FS_DIRECTORY;
                fnode->ops.mkdir   = (mkdir_type_t)tmpfs_mkdir;
                fnode->ops.create  = (create_type_t)tmpfs_create;
                fnode->ops.unlink  = (unlink_type_t)tmpfs_unlink;
                fnode->ops.readdir = (readdir_type_t)tmpfs_readdir;
                fnode->ops.finddir = (finddir_type_t)tmpfs_finddir;

            }
            return fnode;
        }
    } 

    return NULL;
}



struct dirent * tmpfs_readdir(fs_node_t *node, uint32_t index) {
    // //fb_console_printf("tmpfs_readdir: index:%u:%s\n", index, node->name);
    

    if(node->flags != FS_DIRECTORY){
		return NULL;
	}	

    tmpfs_device_t* tmpfs = node->device;
    if(!tmpfs){        
        return NULL;
    }

    tree_node_t* tnode = tmpfs->self;
    
    if(index < tnode->children->size){
        

        listnode_t* cnode = tnode->children->head;

        for(uint32_t i = 0; i < index; ++i){
            cnode = cnode->next;
        }

        tree_node_t* ctnode = cnode->val;
        tmpfs_device_t* fnode = ctnode->value;
        

        struct dirent* out = kcalloc(1, sizeof(struct dirent));
        out->ino = index;
        out->type = fnode->type;
        out->off = index;
        out->reclen = 0;
        strcpy(out->name, fnode->name);

        node->offset++;
        return out;
    }
    
    node->offset = 0;
    return NULL;
}


void tmpfs_create(fs_node_t* node, char* name, uint16_t permissions){


    //create a new tmpfs entry
    tmpfs_device_t* croot = kcalloc(1, sizeof(tmpfs_device_t));
    croot->name = strdup(name);
    croot->type = FS_FILE;
    
    //get reference for all tree and parent node;
    tmpfs_device_t* nodes = node->device;
    tree_t *tree = nodes->tree;
    tree_node_t *leaf = nodes->self;
    
    croot->tree = tree;
    croot->self = tree_node_insert_child(tree, leaf, croot);
    return;
}


void tmpfs_mkdir(fs_node_t* node, char* name, uint16_t permissions){

    //fb_console_printf("tmpfs_mkdir: %s, %x\n", name, permissions);
    
    //create a new tmpfs entry
    tmpfs_device_t* croot = kcalloc(1, sizeof(tmpfs_device_t));

    //get reference for all tree and parent node;
    tmpfs_device_t* nodes = node->device;
    tree_t *tree = nodes->tree;
    tree_node_t *leaf = nodes->self;
    
    
    croot->name = strdup(name);
    croot->type = FS_DIRECTORY;
    croot->tree = tree;
    croot->self = tree_node_insert_child(tree, leaf, croot);
    return;
}

void tmpfs_open(fs_node_t* node, int flags){

    int read  = flags & O_RDONLY;
    int write = flags & O_WRONLY;
    node->ops.read  = (read ?  tmpfs_read  : NULL);
    node->ops.write = (write_type_t)(write ? tmpfs_write : NULL);
    node->ops.truncate = (truncate_type_t)tmpfs_truncate;

    tmpfs_device_t* n = node->device;
    n->ref_count++;
}

void tmpfs_close(fs_node_t* node){
    
    tmpfs_device_t* n = node->device;
    n->ref_count--;
    // kfree(node);
    return;
}



uint32_t tmpfs_write(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){
    
  
    tmpfs_device_t* tmpfs = node->device;

    uint32_t end_offset = offset + size;

    //Pad with zeros if offset is beyond current data_size
    if (offset > tmpfs->data_size) {
        uint32_t pad_size = offset - tmpfs->data_size;
        uint8_t zero = 0;

        for (uint32_t i = 0; i < pad_size; i++) {
            tmpfs_write(node, tmpfs->data_size + i, 1, &zero);
        }
    }

    if (end_offset > tmpfs->data_size) {
        node->length = tmpfs->data_size;
        tmpfs->data_size = end_offset;
    }

    int page_index = offset / 4096;
    int page_offset = offset % 4096;
    // if(size > 1){

    //     //fb_console_printf("tmpfs_write: page_index:%u page_offset:%u\n", page_index, page_offset);
    // }
    listnode_t* page = tmpfs->data_page_list.head;

    if (!page) {
        
        uint8_t* npage = kpalloc(1);
        list_insert_end(&tmpfs->data_page_list, npage);
        page = tmpfs->data_page_list.head;
    }

    for (int i = 0; i < page_index; ++i) {
        if (!page->next) {

            uint8_t* page = kpalloc(1);
            list_insert_end(&tmpfs->data_page_list, page);
        }
        page = page->next;
    }

    size_t left = size;
    while (left--) {
        uint8_t *wp = page->val;
        wp[page_offset++] = *(buffer++);  // Write data

        if (page_offset >= 4096) {
            page_offset = 0;

            // Allocate next page if needed
            if (!page->next) {

                uint8_t* page = kpalloc(1);
                list_insert_end(&tmpfs->data_page_list, page);
            }

            page = page->next;
        }
    }

    return size;
}

uint32_t tmpfs_read(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){

    tmpfs_device_t* tmpfs =  node->device;

    if(offset >= tmpfs->data_size){
        return 0;
    }

    size_t data_left =  tmpfs->data_size - offset;
    if(size > data_left){
        size = data_left;
    }

    int page_index = offset / 4096;
    int page_offset = offset % 4096;
    
    // if(size > 1){

    //     //fb_console_printf("tmpfs_read: page_index:%u page_offset:%u\n", page_index, page_offset);
    // }

    listnode_t* page = tmpfs->data_page_list.head;
    for(int i = 0; i < page_index; ++i) page = page->next;

    size_t left = size;
    while(left--){
        
        uint8_t *rp = page->val;
        *(buffer++) = rp[page_offset++];

        if(page_offset >= 4096){
            
            page_offset = 0;
            page_index++;
            page = page->next;
        }
    }

    return size;
}



void tmpfs_truncate(fs_node_t* n, size_t s){
    tmpfs_device_t* tmpfs = n->device;

    if(s > tmpfs->data_size){ //extend with zero
        
        int old_block_count = tmpfs->data_size / 4096;
        int old_offset      = tmpfs->data_size % 4096;
        int old_round = old_block_count + (old_offset != 0);
        
        int new_block_count = s / 4096;
        int new_offset      = s % 4096;
        int new_round = new_block_count + (new_offset != 0);

        for(int i = 0; i < (new_round - old_round); ++i ){

            list_insert_end(&tmpfs->data_page_list, kpalloc(1));
        }

        tmpfs->data_size = s;
    }
    else{ //reduce it
        int block_count = s / 4096;
        int offset = s % 4096;
        list_t original_list = tmpfs->data_page_list;
        
        tmpfs->data_size = s;
        tmpfs->data_page_list = list_create();

        for(int i = 0; i < block_count; ++i){
            listnode_t* ln = list_remove(&original_list, original_list.head);
            list_insert_end(&tmpfs->data_page_list, ln->val);
            kfree(ln);
        }

        if(offset > 0){
            listnode_t* ln = list_remove(&original_list, original_list.head);
            list_insert_end(&tmpfs->data_page_list, ln->val);
            kfree(ln);
        }

        //free the remaining data in the list
        for(listnode_t* node = list_remove(&original_list, original_list.head) ; node ; node = list_remove(&original_list, original_list.head)){

            void* page = node->val;
            kpfree(page);
            kfree(node);
        }

    }   
    
}

int tmpfs_unlink(fs_node_t* node, char* name){

    tmpfs_device_t* tmpfs = node->device;
    tree_node_t* tnode = tmpfs->self;

    for(listnode_t* node = tnode->children->head; node ; node = node->next){

        tree_node_t* ctnode = node->val;
        tmpfs_device_t* _tmpfs = ctnode->value;
            
        if(!strcmp(_tmpfs->name, name) && _tmpfs->type & FS_FILE){
            
            listnode_t* holdingnode = list_remove(tnode->children, node);
            tree_node_t* holdingtnode = holdingnode->val;
            tmpfs_device_t* tmpfs = holdingtnode->value;

            //well deallocation time
            kfree(holdingnode);
            kfree(holdingtnode->children);
            kfree(holdingtnode);

            for(listnode_t* node = list_pop_end(&tmpfs->data_page_list);  node ; node = list_pop_end(&tmpfs->data_page_list) ){
				
                kpfree(node->val);
                kfree(node);
			}

            kfree(tmpfs->name);
            kfree(tmpfs);
            break;
        }
    }
    return 0;
}
