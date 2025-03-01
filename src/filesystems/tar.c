#include <filesystems/tar.h>
#include <uart.h>

static const char * type_table[23] = {
    "Regular File",
    "Link File",
    "Reserved 2",
    "Character Special File",
    "Block Special File",
    "Directory",
    "FIFO Special File",
    "Reserved 7"
	};
    

static int tar_filetype_vfs_type[9] = {
    FS_FILE,
    FS_SYMLINK,
    -1,
    FS_CHARDEVICE,
    FS_BLOCKDEVICE,
    FS_DIRECTORY,
    FS_PIPE,
    -1
};

static tar_header_t * tar;
int o2d(const char *in)
{
    if(in[0] == '\0'){
        return -1;
    }

    unsigned int j;
    for(j = 0; in[j] != '\0'; ++j);
    unsigned int size = 0;
    unsigned int count = 1;
 
    for (/*j = 11*/; j > 0; j--, count *= 8)
        size += ((in[j - 1] - '0') * count);
 
    return size;
 
}

void tar_add_source(void * src){
  tar = src;
  return;
}
tar_header_t *tar_get_file(const char * path, int mode){
    (void)mode;
    /*
    parse filepatto directories 
    -> /dev/share/consolefonts/screenfonts.psf
    ->/ dev share consolefonts screenfonts.psf
    or just add "./" at the beginning and just search it through?
    but i don't have dynamic memory allocator 
    or just ignore the "." at the beginning in the tar pathnames
    */ 
   tar_header_t * ans = NULL;
    for(int i = 0; ; ++i){
		
		if(tar[i].filename[0] == '\0'){
			break;
		}

        if( memcmp(path, &tar[i].filename[1], strlen(path)) == 0){
            ans = &tar[i];
            break;
        }
        int size = o2d(tar[i].size);
        i += (size / 512) + (size % 512 != 0);//ceil //next header

		
	}
    return ans;

}

void tar_parse(){
    for(int i = 0; ; ++i){
		
		if(tar[i].filename[0] == '\0'){
			break;
		}
		
		int size = o2d(tar[i].size);
		uart_print(COM1, "%s\r\n", tar[i].filename);
		uart_print(COM1, "size : %x \r\n", size);
		uart_print(COM1, "type : %s\r\n", type_table[ tar[i].typeflag[0] - '0']);
		uart_print(COM1, "device major number : %s\r\n", tar[i].devmajor);
		uart_print(COM1, "device minor number : %s\r\n", tar[i].devminor);
		uart_print(COM1, "\r\n");
        i += (size / 512) + (size % 512 != 0);//ceil

		
	}

}

static tar_header_t * tar_next(tar_header_t * head){
    
    if(head == NULL){
        return NULL;
    }

    if(head->filename[0] == '\0')
        return NULL;
    
    int size = o2d(head->size);
    int offset = 1 + (size / 512) + ( size % 512 != 0); //like ceil
    
    
    if(head[offset].filename[0] == '\0')
        return NULL;

    return &head[offset];

}

file_types_t tar_get_filetype(tar_header_t * tar){
    return tar->typeflag[0] -'0';
}

int tar_get_major_number(tar_header_t * t){
    if(tar_get_filetype(t) == REGULAR_FILE) return -1;
    
    return o2d(t->devmajor);
    // return t->devmajor[6] - '0';
}

int tar_get_minor_number(tar_header_t * t){
    if(tar_get_filetype(t) == REGULAR_FILE) return -1;
    return o2d(t->devminor);
    // return t->devminor[6] - '0';
}





/*
    Few assumptions:
        -> it seems tar places directories and its contents like
            Dir1 : 
                Dir2:
                    file4
                    file5

                file1
                file2
                file3

    so when given a directory node(?) i can use offset flag to scan the files and dirs.
    altough dirs are listed first we can pass over them by the name, since names are
    root_dir/file* or root_dir/dir*_/ for dirs each time i had to pass over them but anyway
    
            
*/
int32_t tar_read_dir(file_t * dir, tar_header_t ** out){
    
    tar_header_t * tar = dir->f_inode;
    if(tar_get_filetype(tar) != DIRECTORY){
        return -1;
    }
    
    // fb_console_printf("file_t dir: %u\n", dir->f_pos);

    for(long off = 0; off < dir->f_pos; ++off){


        tar = tar_next(tar);
        if(!tar){ //null
            return -1;
        }

        //outside the dir?
        if( strncmp(&dir->f_inode->filename[1], &tar->filename[1], strlen(&dir->f_inode->filename[1]) ) ) return -1;

        //for instance we find a file within the subfolder, means we should pass over it
        //since it seems subfolders are listed first thus we will be here in the 2nd pass instead of returning lets decrement i
        //for detecting, scan for "/" character for the pathnames of files sliced from dir to end of the text
        //if we find second one we pass
        int limit =  tar_get_filetype(tar) != DIRECTORY ? 0 : 1;
        
        if( tar_get_filetype(tar) != DIRECTORY || 1){

            char * head = tar->filename;
            head += strlen(dir->f_inode->filename);
            if( is_char_in_str('/', head) != limit) off--;
            // fb_console_printf("%s : %u\n", head, is_char_in_str('/', head));

        }

    }




        if(out) 
            *out = tar;
        else
        {
            tar_header_t * d = tar;
            int size = o2d(d->size);
		    fb_console_printf("%s\n", d->filename);
		    fb_console_printf("size : %x\n", size);
		    fb_console_printf("type : %s\n", type_table[ tar_get_filetype(d) ]);    
        }
	

        // dir->f_pos += 1;
    return 0;
}



static struct dirent * tar_readdir_initrd(fs_node_t *node, uint32_t index) {
	
    // fb_console_printf("tar_readdir_initrd: index:%u:%s\n", index, node->name);
    
    if(node->flags != FS_DIRECTORY){
        return NULL;
    }

    //nevermind previous assumption, root directorys and its sub directories are in a list so
    tar_header_t* head = tar_get_file(node->name, 0);
    // if(!head){
    //     fb_console_printf("tar readdir failed to get file %s\n", node->name);
    //     return NULL;
    // }

    
    head = tar_next(head);

    for(int l_index = 0; l_index < index ; l_index++){

        head = tar_next(head);

        if(head && !strncmp(node->name, &head->filename[1], strlen(node->name)) ){
            
            char * rpath = head->filename + 1 + strlen(node->name);
            int slash_count =  is_char_in_str('/', rpath);

            if(tar_get_filetype(head) == DIRECTORY ){
                
                if(slash_count != 1){

                    l_index--;
                    continue;
                }
            }
            else{
                if(slash_count != 0){

                    l_index--;
                    continue;
                }
            }
            
            
            // fb_console_printf("\t->%s::%u::%s\n", rpath, slash_count, type_table[tar_get_filetype(head)]);
            continue;
        }

        //no other
        node->offset = 0;
        return NULL;
    }


    struct dirent* out = kcalloc(1, sizeof(struct dirent));
    out->ino = index;
    out->type = tar_filetype_vfs_type[tar_get_filetype(head)];
    out->off = index;
    out->reclen = 0;
    strcpy(out->name, &head->filename[strlen(node->name) + 1]);
    if(out->name[strlen(out->name) - 1] == '/') out->name[strlen(out->name) - 1] = '\0';
    // memcpy(out->name, &head->filename[strlen(node->name) + 1], strlen(node->name));

    node->offset++;
    return out;

}

static read_type_t tar_read_initrd(struct fs_node *node , uint32_t offset, uint32_t size, uint8_t * buffer){

        tar_header_t* thead = node->device;

        if(offset >= o2d(thead->size)){ //EOF
            return 0;
        }
        else{
        
            uint8_t  * result;
            result = (uint8_t*)(&(thead[1]));
            memcpy(buffer, &result[offset], size);

            node->offset += size;
            return size;
        }
}


static uint32_t tar_indode = 0;
finddir_type_t tar_finddir_initrd (struct fs_node* node, char *name){
    //from here name should be ../dir/
    // fb_console_printf("tar_finddir_initrd: path: %s:%s\n", node->name, name);

    //hmm needs / it seems
    char* nname;

    nname = kcalloc(strlen(node->name) + strlen(name) + 1, 1);
    

    strcat(nname, node->name);    
    strcat(nname, name);
    
    
    // fb_console_printf("new path : %s\n", nname );
    tar_header_t * file = tar_get_file(nname, 0);

    kfree(nname);
    
    if(file){
        // fb_console_printf("found:%s\n", file->filename);
        struct fs_node * fnode = kcalloc(1, sizeof(struct fs_node));
        fnode->inode = tar_indode++;
        strcpy(fnode->name, &file->filename[1]);
        fnode->uid = o2d( file->uid);
	    fnode->gid = o2d(file->gid);
        fnode->impl = 0;
        fnode->device = file;
        
        
        

        switch(tar_get_filetype(file)){
            
            case DIRECTORY:
                fnode->flags   = FS_DIRECTORY;
	            fnode->read    = NULL;
	            fnode->write   = NULL;
	            fnode->open    = NULL;
	            fnode->close   = NULL;
	            fnode->readdir = tar_readdir_initrd;
	            fnode->finddir = tar_finddir_initrd;
	            fnode->ioctl   = NULL;
                break;
            
            case REGULAR_FILE:
                fnode->length = o2d(file->size);
                fnode->flags   = FS_FILE;
	            fnode->read    = tar_read_initrd;
	            fnode->write   = NULL;
	            fnode->open    = NULL;
	            fnode->close   = NULL;
	            fnode->readdir = NULL;
	            fnode->finddir = NULL;
	            fnode->ioctl   = NULL;
                break;
         
            case CHARACTER_SPECIAL_FILE:
                fnode->flags = FS_CHARDEVICE;
                fnode->major_number = tar_get_major_number(file);
                fnode->minor_number = tar_get_minor_number(file);
                
                
                break;
            default:
                break;

        }

        
        
        fnode->offset = 0;
        return fnode;
        
    }

    return NULL;

}
 


//created during the mount and will be freed when umount is called
fs_node_t * tar_node_create(void * tar_begin, size_t tar_size){
	fs_node_t * fnode = kcalloc(1, sizeof(fs_node_t));

	fnode->inode = tar_indode++;
	strcpy(fnode->name, "/");
    fnode->device = tar_begin;
    fnode->impl = 0; //tar file
	fnode->uid = 0;
	fnode->gid = 0;
	fnode->flags   = FS_DIRECTORY;
	fnode->read    = NULL;
	fnode->write   = NULL;
	fnode->open    = NULL;
	fnode->close   = NULL;
	fnode->readdir = tar_readdir_initrd;
	fnode->finddir = tar_finddir_initrd;
	fnode->ioctl   = NULL;
    
	return fnode;
}


#include <filesystems/tmpfs.h>
// fs_node_t* tar_convert_tmpfs(void* tar_begin, uint32_t binary_size){

//     tar_header_t* tar = tar_begin;
//     fs_node_t* tmpfs_root = tmpfs_install();
//     tar = tar_next(tar);
//     for(tar_header_t* head = tar;  head ; head = tar_next(head)){
        

//         int is_dir = head->filename[strlen(head->filename) - 1] == '/';
//         fb_console_printf(
//             "->%s %s-> ", head->filename,    
//             is_dir ? "dir"  : "file"
//             );
        
//         char** pieces = _vfs_parse_path(&head->filename[2]);
//         for(int i = 0; pieces[i] ; i++){
//             fb_console_printf("%s ", pieces[i]);
//         }
//         fb_console_putchar('\n');


//         //doing work

//         fs_node_t* rnode = tmpfs_root;
//         for(int i = 0; pieces[i] ; i++){
            
//             if(!pieces[i + 1]){
                
//                 switch (tar_filetype_vfs_type[tar_get_filetype(head)]){

//                     case FS_DIRECTORY:
//                         mkdir_fs(rnode, pieces[i], 0644);
//                         break;

//                     case FS_FILE:
//                         create_fs(rnode, pieces[i], 0x644);
//                         fs_node_t* cnode = finddir_fs(rnode, pieces[i]);

//                         struct tmpfs_internal_struct* tis = cnode->device;
// 				        tis->priv = kmalloc( o2d(head->size) );
// 				        tis->buffer_size =  o2d(head->size);

//                         memcpy(tis->priv, &head[1], o2d(head->size));
//                         cnode->length = o2d(head->size);
//                         // write_fs(cnode, 0, o2d(head->size), &head[1]);

//                         break;

//                     default:break;
                    
//                 }
//                 break;
//             }

//             //check whether first folders exist for other entries   
//             fs_node_t* dnode = finddir_fs(rnode, pieces[i]);
//             if(!dnode){
//                 mkdir_fs(rnode, pieces[i], 0644);
//                 dnode = finddir_fs(tmpfs_root, pieces[i]);
//             }  

//             rnode = dnode;


//         }






//     }

//     return tmpfs_root;

// }