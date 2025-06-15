#ifndef __STAT_H__
#define __STAT_H__

// #include <dirent.h>

#include <stdint-gcc.h>
typedef struct  {
   unsigned int   st_dev;      /* ID of device containing file */
   unsigned int   st_ino;      /* Inode number */
   unsigned int  st_mode;     /* File type and mode */
   unsigned int st_nlink;    /* Number of hard links */
   unsigned int   st_uid;      /* User ID of owner */
   unsigned int   st_gid;      /* Group ID of owner */

   unsigned int      st_rdev;     /* Device ID (if special file) */
   uint32_t      st_size;     /* Total size, in bytes */
   uint32_t  st_blksize;  /* Block size for filesystem I/O */
   uint32_t   st_blocks;   /* Number of 512 B blocks allocated */


   uint32_t  st_atime;  /* Time of last access */
   uint32_t  st_mtime;  /* Time of last modification */
   uint32_t  st_ctime;  /* Time of last status chiange */

} stat_t;


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


#endif