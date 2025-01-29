#ifndef __DIRENT_H__
#define __DIRENT_H__

#include <stddef.h>
#include <stdint-gcc.h>


#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x04
#define FS_BLOCKDEVICE 0x08
#define FS_PIPE        0x10
#define FS_SYMLINK     0x20
#define FS_MOUNTPOINT  0x40


struct dirent {
	uint32_t d_ino;           // Inode number.
    uint32_t d_off;
    uint16_t d_reclen;
    int32_t d_type;
	char d_name[256];         // The filename.
};


#endif