#include <filesystems/tar.h>


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