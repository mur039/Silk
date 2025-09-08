#include <filesystems/fs.h>
#include <filesystems/vfs.h>



struct fs_node*  pts_create_node();

#define TIOCGPTN  0x5430  // get pty number
#define TIOCSPTLCK 0x5431 //set lock
#define TIOCGPTLCK 0x5432 //getlock

