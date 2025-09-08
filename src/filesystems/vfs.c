#include <filesystems/vfs.h>
#include <g_tree.h>
#include <uart.h>
#include <module.h>

char** _vfs_parse_path(const char * path){
    
    list_t* paths;
    paths = kcalloc(1, sizeof(list_t));
    paths->size = 0;
    paths->head = NULL;
    paths->tail = NULL;

    const char * previous_head = path;
    for(int i = 0; ; ++i){

    

        if(path[i] == '\0'){

            if(previous_head != &path[i]){

                size_t size = (size_t)(&path[i] - previous_head);
                char * name = kcalloc(1,size + 1);
                memcpy(name, previous_head, size);

                // uart_print(COM1, "%s\r\n", name);
                list_insert_end(paths, name);

            }
            break;
        }

        if(path[i] == '/'){
            
            size_t size = (size_t)(&path[i] - previous_head);
            // if(!i) size += 1;
            char * name = kcalloc(1,size + 1);
            memcpy(name, previous_head, size);

            // uart_print(COM1, "%s\r\n", name);
            list_insert_end(paths, name);
            
            previous_head = &path[i + 1];            
        }
        

    }
    
    
    //printing the list and construct an array of strings;
    char** retval = kcalloc(paths->size + 1, sizeof(char*));
    char** head = retval;
    
    for(listnode_t * node = paths->head;  node != NULL; node = node->next){
        
        char * val = node->val;
        *(head++) = val;
        *head = NULL;
    }


    //purge the list
    for(listnode_t * node = list_remove(paths, paths->head);  node ; node = list_remove(paths, paths->head)){
        //dont'free the texts
		kfree(node);
    }
    kfree(paths);


    return retval;
}



// tree_t * vfs_tree = NULL;

// struct superblock *root_sb = NULL;
// struct dentry *root_dentry = NULL;


// struct superblock* vfs_get_root_superblock(){
//     return root_sb;
//     }

// struct dentry*vfs_get_root_dentry(){
//     return root_dentry;
// }

// void vfs_init() {
//     root_sb = kmalloc(sizeof(struct superblock));
//     memset(root_sb, 0, sizeof(struct superblock));

//     root_dentry = kmalloc(sizeof(struct dentry));
//     memset(root_dentry, 0, sizeof(struct dentry));

//     root_dentry->d_inode = NULL; // No inode yet
//     root_dentry->d_name = "/";   // Root directory
//     root_dentry->parent = NULL;  // Root has no parent

// }



// static struct file_system_type *registered_filesystems = NULL;

// void register_file_system(struct file_system_type *fs) {
//     fs->next = registered_filesystems;
//     registered_filesystems = fs;
// }

// struct file_system_type *get_file_system(const char *name) {
//     struct file_system_type *fs = registered_filesystems;
//     while (fs) {
//         if (strcmp(fs->name, name) == 0) {
//             return fs;
//         }
//         fs = fs->next;
//     }
//     return NULL; // File system not found
// }


// struct dentry *resolve_path(const char *path) {
//     // struct dentry *current = root_dentry; // Start at root
//     // char component[256];
//     // while (get_next_component(path, component)) {
//     //     current = current->d_inode->i_ops->lookup(current, component);
//     //     if (!current) return NULL; // Path not found
//     // }
//     // return current;
// }

// void vfs_traverse_tree(struct dentry* node){

    

// }




// //fuck

// uint32_t read_fs(fs_node_t *node, u32int offset, u32int size, u8int *buffer)
// {
//   // Has the node got a read callback?
//   if (node->read != 0)
//     return node->read(node, offset, size, buffer);
//   else
//     return 0;
// } 



tree_t    * fs_tree = NULL; /* File system mountpoint tree */
fs_node_t * fs_root = NULL; /* Pointer to the root mount fs_node (must be some form of filesystem, even ramdisk) */

struct fsnode_cache fs_cache;
static size_t inode_hash(void* mountpoint, size_t inode, size_t bucket_count) {
    // combine mount pointer + inode_id into a single hash
    uintptr_t mount_val = (uintptr_t)mountpoint;
    return ((mount_val >> 3) ^ inode) % bucket_count;
}

void fsnode_cache_insert(struct fsnode_cache *cache, struct fs_node *node, struct fs_node *mount, uint32_t inode) {
    size_t idx = inode_hash(mount, inode, cache->bucket_size);

    struct fsnode_cache_entry *new_entry = kcalloc(1, sizeof(*new_entry));
    new_entry->node = node;
    new_entry->key = idx;
    
    new_entry->next = cache->buckets[idx];
    cache->buckets[idx] = new_entry;
}



void vfs_install() {
	/* Initialize the mountpoint tree */
	fs_tree = tree_create();

	struct vfs_entry * root = kmalloc(sizeof(struct vfs_entry));
	if(!root)
		error("failed allocate for vfs_entry");
	root->name = strdup("[root]");
	root->nodes = list_create(); /* Nothing mounted as root */
	list_insert_end(&root->nodes, NULL);

	tree_set_root(fs_tree, root);

	//initiailize the cache
	fs_cache.bucket_size = 256;
	fs_cache.buckets = kcalloc(256, sizeof(struct fsnode_cache_entry));


}

#include <fb.h>
char* vfs_canonicalize_path(const char* cwd, const char* rpath){
    // fb_console_printf("vfs_canonicalize_path:: cwd:%s rpath:%s\n", cwd, rpath);
    
    char* concat_path = NULL;

    if(rpath[0] == PATH_SEPARATOR){ //rpath is actually absolute
        concat_path = strdup(rpath);
    }
    else{
        concat_path = kcalloc( strlen(cwd) + strlen(rpath) + 1, 1);
        strcat(concat_path, cwd);
        strcat(concat_path, rpath);
    }



    list_t path_piece_list = {.head = NULL, .tail = NULL, .size = 0};
    char** concat_path_pieces = _vfs_parse_path(concat_path);

    for(int i = 0; concat_path_pieces[i]; ++i){
        // fb_console_printf("->%s\n", concat_path_pieces[i]);

        if(!memcmp(concat_path_pieces[i], PATH_DOT, 2)){
            continue;
        }
        else if(!strncmp(concat_path_pieces[i], PATH_UP, 2)){
            if(!list_pop_end(&path_piece_list)){ //null so no element left
                break;
            }
            continue;
        }

        list_insert_end(&path_piece_list, concat_path_pieces[i] );
    }
    kfree(concat_path_pieces);


    // fb_console_printf("--------------------------------\n");

    if(!path_piece_list.size)
        list_insert_end(&path_piece_list, "/");
    

    memset(concat_path, 0, strlen(concat_path));
    char* phead = concat_path;
    for(listnode_t* node = path_piece_list.head; node ; node = node->next){
        

        if(node != path_piece_list.head) strcat(concat_path, "/");
        strcat(concat_path, node->val);
        
        
    }



    return concat_path;
}


int vfs_mount(char * path, fs_node_t * local_root){

    if (!fs_tree) {
		fb_console_printf("VFS hasn't been initialized, you can't mount things yet!");
		return 1;
	}

    	if (!path || path[0] != '/') {
		fb_console_printf("Path must be absolute for mountpoint.");
		return 2;
	}

	int ret_val = 0;

	char * p = strdup(path);
	char * i = p;

	int path_len   = strlen(p);

	/* Chop the path up */
	while (i < p + path_len) {
		if (*i == PATH_SEPARATOR) {
			*i = '\0';
		}
		i++;
	}
	/* Clean up */
	p[path_len] = '\0';
	i = p + 1;

	/* Root */
	tree_node_t * root_node = fs_tree->root;

	if (*i == '\0') {
		/* Special case, we're trying to set the root node */
		struct vfs_entry * root = (struct vfs_entry *)root_node->value;

		// if (root->file) {
		// 	fb_console_printf("Path %s already mounted, unmount before trying to mount something else.\n", path);
		// 	ret_val = 3;
		// 	goto _vfs_cleanup;
		// }
		list_insert_end(&root->nodes, local_root);
		// root->file = local_root;
		/* We also keep a legacy shortcut around for that */
		fs_root = local_root;

	} else {
		tree_node_t * node = root_node;
		char * at = i;
		while (1) {
			if (at >= p + path_len) {
				break;
			}
			int found = 0;
			// fb_console_printf("Searching for %s\n", at);
            
			// foreach(child, node->children) 
            for(listnode_t* child = node->children->head; child ; child = child->next )
            {
				tree_node_t * tchild = (tree_node_t *)child->val;
				struct vfs_entry * ent = (struct vfs_entry *)tchild->value;
				if (!strcmp(ent->name, at)) {
					found = 1;
					node = tchild;
					break;
				}
			}
			if (!found) {
				fb_console_printf( "Did not find %s, making it.\n", at);
				struct vfs_entry * ent = kcalloc(1, sizeof(struct vfs_entry));
				if(!ent)
					error("failed allocate for vfs_entry");

				ent->name = strdup(at);
				// ent->file = NULL;
				ent->nodes = list_create();
				node = tree_node_insert_child(fs_tree, node, ent);
			}
			at = at + strlen(at) + 1;
		}

		struct vfs_entry * ent = (struct vfs_entry *)node->value;
		// if (ent->file) {
		// 	fb_console_printf( "Path %s already mounted, unmount before trying to mount something else.\n", path);
		// 	ret_val = 3;
		// 	goto _vfs_cleanup;
		// }
		list_insert_end(&ent->nodes, local_root);
		// ent->file = local_root;
	}

_vfs_cleanup:
	kfree(p);
	return ret_val;


}


static void vfs_tree_traverse_helper_func( tree_node_t * node, int height){ //smart
    
    if (!node) return; //for invalid and stop recursion
    
    //indentation
    for(int i = 0; i < height; ++i){
        fb_console_put("  ");
    }
    
    struct vfs_entry* entry = node->value;
    // if(entry->file){ //mounted
		fs_node_t* fnode = entry->nodes.tail->val;
        fb_console_printf("->name:%s file:%s node_name:%s\n", entry->name, fnode->name , fnode->name );
    // }
    // else{
    //     fb_console_printf("->name:%s file:(empty)\n", entry->name );
    // }

    for(listnode_t* lnode = node->children->head; lnode ; lnode = lnode->next){
        vfs_tree_traverse_helper_func(lnode->val, height + 1);
    }


}

void vfs_tree_traverse(tree_t* tree){
    vfs_tree_traverse_helper_func(tree->root, 0);
}




fs_node_t *get_mount_point(char * path, unsigned int path_depth, char **outpath, unsigned int * outdepth) {
	
	// fb_console_printf("get_mount_point :%s\n", path);
	size_t depth;

	for (depth = 0; depth <= path_depth; ++depth) {
		path += strlen(path) + 1;
	}

	/* Last available node */
	fs_node_t   * last = fs_root;
	tree_node_t * node = fs_tree->root;

	char * at = *outpath;
	int _depth = 1;
	int _tree_depth = 0;

	while (1) {
		if (at >= path) {
			break;
		}
		int found = 0;
		// fb_console_printf("Searching for %s\n", at);

		
        for(listnode_t* child = node->children->head;  child  ; child = child->next)
        {
			tree_node_t * tchild = (tree_node_t *)child->val;
			struct vfs_entry * ent = (struct vfs_entry *)tchild->value;
			if (!strcmp(ent->name, at)) {
				found = 1;
				node = tchild;
				at = at + strlen(at) + 1;
				// if (ent->file) {
				if (ent->nodes.tail) {
					_tree_depth = _depth;
					last = ent->nodes.tail->val;
					*outpath = at;
				}
				break;
			}
		}

		if (!found) {
			break;
		}
		_depth++;
	}

	*outdepth = _tree_depth;

	return last;
}




uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	struct fs_ops *ops = &node->ops;
	if (ops->read) {
		uint32_t ret = ops->read(node, offset, size, buffer);
		return ret;
	} else {
		return 0;
	}
}

EXPORT_SYMBOL(read_fs);

uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	struct fs_ops *ops = &node->ops;
	if (ops->write) {
		uint32_t ret = ops->write(node, offset, size, buffer);
		return ret;
	} else {
		return size;
	}
}

int unlink_fs(fs_node_t* node, const char* name){
	struct fs_ops *ops = &node->ops;
	if(node->flags & FS_DIRECTORY && ops->unlink)
	{
		return ops->unlink(node, name);
	}
	else{
		return -1;
	}

}



void create_fs(fs_node_t* node,  const char *name, uint16_t permissions){
	struct fs_ops *ops = &node->ops;
    if(node->flags & FS_DIRECTORY &&  ops->create){
        ops->create(node, name, permissions);
    }
}



int mkdir_fs(fs_node_t* node, char *name, uint16_t permission){
	struct fs_ops *ops = &node->ops;
    if(node->flags & FS_DIRECTORY &&  ops->mkdir){
        ops->mkdir(node, name, permission);
		return 1;
    }
	return 0;
}



fs_node_t *finddir_fs(fs_node_t *node, char *name) {

	struct fs_ops *ops = &node->ops;
	if ((node->flags & FS_DIRECTORY) && ops->finddir) {
		fs_node_t *ret = ops->finddir(node, name);
		return ret;
	} else {
		fb_console_printf("Node passed to finddir_fs isn't a directory!\n");
		fb_console_printf("node = 0x%x:%s, name = %s\n", node, node->name, name);
		return (fs_node_t *)NULL;
	}
}


struct dirent *readdir_fs(fs_node_t *node, uint32_t index) {
	struct fs_ops *ops = &node->ops;
    if ((node->flags & FS_DIRECTORY) && ops->readdir) {
		struct dirent *ret = ops->readdir(node, index);

		return ret;
	} else {
		return (struct dirent *)NULL;
	}
}


void open_fs(fs_node_t *node, int flags) {
	struct fs_ops *ops = &node->ops;
	if (ops->open) {
		ops->open(node, flags);
	}
	// node->refcount++;
}

void close_fs(fs_node_t *node) {
	struct fs_ops *ops = &node->ops;
	if(node == fs_root){
        uart_print(COM1, "The fuck is wrong with you\n");
    }

	if (ops->close) {
		ops->close(node);	
	}

	// if(node->refcount > 0) node->refcount--;
	// if(node->refcount == 0) kfree(node);

	
}

#include <syscalls.h>

int ioctl_fs(fs_node_t* node, unsigned long req, void* arg){
	struct fs_ops* ops = &node->ops;
	if(ops->ioctl){
		return ops->ioctl(node, req, arg);
	}
	else{
		return -ENOTTY;
	}
}

#include <process.h>
fs_node_t *kopen(char *filename, uint32_t flags) {
	
    if (!fs_root || !filename) {
		return NULL;
	}

	// fb_console_printf("kopen(%s) \n", filename);

	// /* Reference the current working directory */
	char *cwd = (char *)(current_process->cwd);
	// /* Canonicalize the (potentially relative) path... */
	char *path = vfs_canonicalize_path(cwd, filename);
	// /* And store the length once to save recalculations */

    // char* path = strdup( filename);
	size_t path_len = strlen(path);

	/* Otherwise, we need to break the path up and start searching */
	char *path_offset = path;
	uint32_t path_depth = 0;
	while (path_offset < path + path_len) {
		/* Find each PATH_SEPARATOR */
		if (*path_offset == PATH_SEPARATOR) {
			*path_offset = '\0';
			path_depth++;
		}
		path_offset++;
	}
	/* Clean up */
	path[path_len] = '\0';
	path_offset = path + 1;

	/*
	 * At this point, the path is tokenized and path_offset points
	 * to the first token (directory) and path_depth is the number
	 * of directories in the path
	 */

	/*
	 * Dig through the (real) tree to find the file
	 */
	uint32_t depth = 0;
	fs_node_t *node_ptr = kmalloc(sizeof(fs_node_t));
	if(!node_ptr)
		error("failed to allocate for node_ptr");
		
	/* Find the mountpoint for this file */
	fs_node_t *mount_point = get_mount_point(path, path_depth, &path_offset, (void*)&depth);

	if (path_offset >= path+path_len) {
		kfree(path);
		kfree(node_ptr);
		return mount_point;
	}

	/* Set the active directory to the mountpoint */
	memcpy(node_ptr, mount_point, sizeof(fs_node_t));
	fs_node_t *node_next = NULL;
	for (; depth < path_depth; ++depth) {
		/* Search the active directory for the requested directory */
		// fb_console_printf("... Searching for %s\n", path_offset);
		node_next = finddir_fs(node_ptr, path_offset);
		kfree(node_ptr);
		node_ptr = node_next;
		if (!node_ptr) {
			/* We failed to find the requested directory */
			kfree((void *)path);
			return NULL;
		} else if (depth == path_depth - 1) {
			

			open_fs(node_ptr, flags);
			kfree((void *)path);
			return node_ptr;
		}
		/* We are still searching... */
		path_offset += strlen(path_offset) + 1;
	}
	/* We failed to find the requested file, but our loop terminated. */
	kfree((void *)path);
	return NULL;
}


list_t* vfs_directory_entry_list(fs_node_t* src_node){

	list_t* list = kcalloc(1, sizeof(list_t));
    if(!src_node)
        return NULL;

    void (* kprintf)(const char*, ...) = fb_console_printf;

    while(1){

        uint32_t old_offset = src_node->offset;
        struct dirent* item = readdir_fs(src_node, src_node->offset);
        
        if(old_offset > src_node->offset){
            break;
        }

        if(item){
			char* name = strdup(item->name);
			list_insert_end(list, name);
        	kfree(item);
        }
        else{
            break;
        }
    }

	return list;

}


void vfs_copy(fs_node_t* root_node, fs_node_t* target_node, int depth){

    
     while(1){

        unsigned int old_offset = root_node->offset;
        struct dirent* item = readdir_fs(root_node, root_node->offset);
        
        for(int i = 0; i < depth; ++i){
            // fb_console_putchar(' ');
            }
        
        if(old_offset > root_node->offset){
            // fb_console_putchar('\n');
            break;
        }

        
        fs_node_t* rnode;
        fs_node_t* tnode;

        if(item){
            switch(item->type){
                case FS_FILE:
                    fb_console_printf("%s : %s> file\n", __func__,  item->name);
                    
                    //open on root node
                    rnode = finddir_fs(root_node, item->name);
                    open_fs(rnode, O_RDONLY);
                    
                    //create on target node and open it as well
                    create_fs(target_node, item->name, 0777);
                    tnode = finddir_fs(target_node, item->name);
                    open_fs(tnode, O_WRONLY | O_TRUNC);

                    uint32_t offset = 0;
                    uint8_t buffer;
                    while( read_fs(rnode, offset, 1, &buffer) ){
                        write_fs(tnode, offset, 1, &buffer);
                        offset++;
                    }

                    break;
                
                case FS_DIRECTORY:
                    // fb_console_printf("name : %s> dir\n",item->name);
                    
                    //create folder at target filesystem as well
                    mkdir_fs(target_node, item->name, 0777);
                    
                    rnode = finddir_fs(root_node, item->name);
                    tnode = finddir_fs(target_node, item->name);

                    if(!strcmp(root_node->name, rnode->name)){ return;}
                    
                    vfs_copy( rnode, tnode, depth + 1);
                    break;

                default:break;
            }
			
        	kfree(item);
            item = NULL;
        }
        else{
            break;
        }
    }

}


void vfs_traverse_node(fs_node_t* root_node, int depth){

    char* spacing = kcalloc(1, depth + 1);
    for(int i = 0; i < depth; ++i)
        spacing[i] = ' ';
    
    
    
     while(1){

        unsigned int old_offset = root_node->offset;
        struct dirent* item = readdir_fs(root_node, root_node->offset);
        
        if(old_offset > root_node->offset){
            break;
        }

        if(item){
            switch(item->type){
                case FS_FILE:
                    fb_console_printf("%sname : %s> file\n",spacing,  item->name);
                    break;
                
                case FS_DIRECTORY:
                    fb_console_printf("%sname : %s> dir\n",spacing, item->name);
                    fs_node_t* dnode = finddir_fs(root_node, item->name);
                    
                    

                    if(!strcmp(root_node->name, dnode->name)){ return;}

                    vfs_traverse_node( dnode, depth + 1);
                    break;

                default:break;
            }
			
        	kfree(item);
            item = NULL;
        }
        else{
            break;
        }
    }

}

