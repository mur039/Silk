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
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
 
    for (j = 11; j > 0; j--, count *= 8)
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
    parse filepath to directories 
    -> /dev/share/consolefonts/screenfonts.psf
    ->/ dev share consolefonts screenfonts.psf
    or just add "./" at the beginning and just search it through?
    but i don't have dynamic memory allocator 
    or just ignore the "./" in the tar pathnames
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

file_types_t tar_get_filetype(tar_header_t * tar){
    return tar->typeflag[0] -'0';
}
