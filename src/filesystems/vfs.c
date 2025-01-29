#include <filesystems/vfs.h>
#include <g_tree.h>
#include <uart.h>


char** _vfs_parse_path(const char * path){
    
    list_t* paths;
    paths = kcalloc(1, sizeof(list_t));
    paths->size = 0;
    paths->head = NULL;
    paths->tail = NULL;

    char * previous_head = path;
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


void vfs_install() {
	/* Initialize the mountpoint tree */
	fs_tree = tree_create();

	struct vfs_entry * root = kmalloc(sizeof(struct vfs_entry));
	if(!root)
		error("failed allocate for vfs_entry");
	root->name = strdup("[root]");
	root->file = NULL; /* Nothing mounted as root */

	tree_set_root(fs_tree, root);

    

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
		if (root->file) {
			fb_console_printf("Path %s already mounted, unmount before trying to mount something else.\n", path);
			ret_val = 3;
			goto _vfs_cleanup;
		}
		root->file = local_root;
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
			fb_console_printf("Searching for %s\n", at);
            
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
				struct vfs_entry * ent = kmalloc(sizeof(struct vfs_entry));
				if(!ent)
					error("failed allocate for vfs_entry");

				ent->name = strdup(at);
				ent->file = NULL;
				node = tree_node_insert_child(fs_tree, node, ent);
			}
			at = at + strlen(at) + 1;
		}

		struct vfs_entry * ent = (struct vfs_entry *)node->value;
		if (ent->file) {
			fb_console_printf( "Path %s already mounted, unmount before trying to mount something else.\n", path);
			ret_val = 3;
			goto _vfs_cleanup;
		}
		ent->file = local_root;
	}

_vfs_cleanup:
	kfree(p);
	return ret_val;


}


static void vfs_tree_traverse_helper_func( tree_node_t * node, int height){ //smart
    
    if (!node) return; //for invalid and stop recursion
    
    //indentation
    for(uint32_t i = 0; i < height; ++i){
        fb_console_put("  ");
    }
    
    struct vfs_entry* entry = node->value;
    if(entry->file){ //mounted
 
        fb_console_printf("->name:%s file:%s node_name:%s\n", entry->name, entry->file , entry->file->name );
    }
    else{
        fb_console_printf("->name:%s file:(empty)\n", entry->name );
    }

    for(listnode_t* lnode = node->children->head; lnode ; lnode = lnode->next){
        vfs_tree_traverse_helper_func(lnode->val, height + 1);
    }


}

void vfs_tree_traverse(tree_t* tree){
    vfs_tree_traverse_helper_func(tree->root, 0);
}




fs_node_t *get_mount_point(char * path, unsigned int path_depth, char **outpath, unsigned int * outdepth) {
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
		fb_console_printf("Searching for %s\n", at);

		
        for(listnode_t* child = node->children->head;  child  ; child = child->next)
        {
			tree_node_t * tchild = (tree_node_t *)child->val;
			struct vfs_entry * ent = (struct vfs_entry *)tchild->value;
			if (!strcmp(ent->name, at)) {
				found = 1;
				node = tchild;
				at = at + strlen(at) + 1;
				if (ent->file) {
					_tree_depth = _depth;
					last = ent->file;
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
	if (node->read) {
		uint32_t ret = node->read(node, offset, size, buffer);
		return ret;
	} else {
		return 0;
	}
}


uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
	if (node->write) {
		uint32_t ret = node->write(node, offset, size, buffer);
		return ret;
	} else {
		return 0;
	}
}

unlink_type_t unlink_fs(fs_node_t* node, const char* name){
	if(node->flags == FS_FILE && node->unlink)
	{
		return node->unlink(node, name);
	}
	else{
		return -1;
	}

}



void create_fs(fs_node_t* node,  const char *name, uint16_t permissions){
    if(node->flags & FS_DIRECTORY &&  node->create){
        node->create(node, name, permissions);
    }
}



int mkdir_fs(fs_node_t* node, char *name, uint16_t permission){
    if(node->flags & FS_DIRECTORY &&  node->mkdir){
        node->mkdir(node, name, permission);
    }
}



fs_node_t *finddir_fs(fs_node_t *node, char *name) {

	if ((node->flags & FS_DIRECTORY) && node->finddir) {
		fs_node_t *ret = node->finddir(node, name);
		return ret;
	} else {
		fb_console_printf("Node passed to finddir_fs isn't a directory!\n");
		fb_console_printf("node = 0x%x:%s, name = %s\n", node, node->name, name);
		return (fs_node_t *)NULL;
	}
}


struct dirent *readdir_fs(fs_node_t *node, uint32_t index) {
	
    if ((node->flags & FS_DIRECTORY) && node->readdir) {
		struct dirent *ret = node->readdir(node, index);

		return ret;
	} else {
		return (struct dirent *)NULL;
	}
}


void open_fs(fs_node_t *node, uint8_t read, uint8_t write) {
	if (node->open) {
		node->open(node, read, write);
	}
}

void close_fs(fs_node_t *node) {
	if(node == fs_root){
        uart_print(COM1, "The fuck is wrong with you\n");
    }

	if (node->close) {
		node->close(node);
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
			
            
			open_fs(node_ptr, 1, 0);
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

        int old_offset = src_node->offset;
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



#include <filesystems/tmpfs.h>
#include <stdarg.h>



int32_t vfs_copy_node_to_path(fs_node_t* src_node, fs_node_t* recv_node){



	list_t* tar_dir = vfs_directory_entry_list(src_node);
    for(listnode_t* node = tar_dir->head; node ; node = node->next){



        //open the node    
        fs_node_t* fnode = finddir_fs(src_node, node->val);
		
		if(!fnode) continue;
        switch(fnode->flags){
            case FS_DIRECTORY:
				uart_print(COM1, "found directory : %s\n", fnode->name);
				uart_print(COM1, "recursing...\n");

				if(!strcmp(src_node->name, fnode->name)){
					return 1;
				}
				else{
					mkdir_fs(recv_node, node->val, 0644);
					fs_node_t *rnode = finddir_fs(recv_node, node->val);

					vfs_copy_node_to_path( fnode, rnode);
				}
				break;

			case FS_FILE:
				uart_print(COM1, "found file : %s size:%u\n", fnode->name, fnode->length);
				create_fs(recv_node, node->val, 0644);
				fs_node_t* tnode = finddir_fs(recv_node, node->val);

				struct tmpfs_internal_struct* tis = tnode->device;
				tis->priv = kmalloc( fnode->length );
				tis->buffer_size =  (fnode->length );

				read_fs(fnode, 0, fnode->length, tis->priv);
				tnode->length = fnode->length;
			
				
				
				break;

			default:
				break;
                
        }
		kfree(fnode);
    }




	kfree(tar_dir);
	return 0;
}