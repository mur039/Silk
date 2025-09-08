#ifndef __LOOP_H__
#define __LOOP_H__

#include <sys.h>
#include <filesystems/vfs.h>
#include <dev.h>

struct loop_device {
        
    struct fs_node* lo_inode;        // inode of backing file
    int lo_number;                 // device number (loop0, loop1, etc.)
    int lo_flags;                  // LO_FLAGS_* like read-only, etc.
    struct loop_device* next;                // linked list of loop devices
};

//control ioctl
#define LOOP_CTL_GET_FREE 0x4C80

//ioctl

#define LOOP_SET_FD              0x4C00       //Attach a file descriptor to the loop device. The file you open (disk image) is passed via fd.
#define LOOP_CLR_FD              0x4C01       //Detach the file from the loop device. Frees the loop device.
#define LOOP_SET_STATUS          0x4C02       //Configure properties like offset, encryption (if used), block size, read-only mode, etc.
#define LOOP_GET_STATUS          0x4C03       //Retrieve the current configuration of the loop device.
#define LOOP_SET_STATUS64        0x4C04       
#define LOOP_GET_STATUS64        0x4C05
#define LOOP_CHANGE_FD           0x4C06     //Replace the underlying file while keeping the loop device open.
#define LOOP_SET_CAPACITY        0x4C07



#define LOOP_MAX_DEVICE_COUNT 4
void loop_install();









#endif