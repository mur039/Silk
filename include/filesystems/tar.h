#ifndef __TAR__H__
#define __TAR__H__

#include <stdint.h>
#include <sys.h>
#include <pmm.h>    
#include <dev.h>
#include <filesystems/vfs.h>

typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

typedef enum {
    REGULAR_FILE = 0,
    LINK_FILE,
    RESERVED_2,
    CHARACTER_SPECIAL_FILE,
    BLOCK_SPECIAL_FILE,
    DIRECTORY,
    FIFO_SPECIAL_FILE,
    RESERVED_7
} file_types_t;


typedef union
{
	struct{
   	 	char filename[100];
   	 	char mode[8];
   	 	char uid[8];
   	 	char gid[8];
   	 	char size[12];
   	 	char mtime[12];
   	 	char chksum[8];
   	 	char typeflag[1];

		char linkname[100];
		char magic[6];
		char version[2];
		char uname[32];
		char gname[32];
		char devmajor[8];
		char devminor[8];
   	 	};
   	 	char align[512];
} tar_header_t;


typedef long off_t;

typedef struct  {
	unsigned short f_mode;
	unsigned short f_flags;
	tar_header_t *f_inode;
	off_t f_pos;
	device_t ops;

} file_t;

int o2d(const char *in);
int tar_get_major_number(tar_header_t * t);
void tar_add_source(void * src);
tar_header_t *tar_get_file(const char * path, int mode);
void tar_parse();
file_types_t tar_get_filetype(tar_header_t * tar);
int tar_get_major_number(tar_header_t * t);
int tar_get_minor_number(tar_header_t * t);
int32_t tar_read_dir(file_t * dir, tar_header_t ** out);


#endif