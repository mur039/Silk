#ifndef __TAR__H__
#define __TAR__H__

#include <stdint.h>
#include <sys.h>

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


int o2d(const char *in);
void tar_add_source(void * src);
tar_header_t *tar_get_file(const char * path, int mode);
void tar_parse();
file_types_t tar_get_filetype(tar_header_t * tar);
#endif