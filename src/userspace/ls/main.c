#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>


int main(int argc, char** argv){
    
    //i only list the working directory
    int dir_fd = open(".", O_RDONLY);
    if(dir_fd == -1){
        printf("How the fuck current directory is non-openable!!\n");
        return 1;
    }

    while(1){

        size_t ncount;
        struct dirent dir;
        ncount = getdents(dir_fd, &dir, sizeof(struct dirent));

        if(ncount == -1 || ncount == 0) break;

    
        switch (dir.d_type){
            case FS_FILE:
                printf( "<--->\t%s\n", dir.d_name );
                break;

            case FS_SYMLINK:
                printf( "<sym>\t\x1b[1;34m%s\x1b[0m\n", dir.d_name );
                break;

            case FS_BLOCKDEVICE:
                printf( "<blk>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                break;

            case FS_CHARDEVICE:
                printf( "<chr>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                break;

            case FS_PIPE:
                printf( "<pip>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                break;
            
            case FS_DIRECTORY:
                printf( "<dir>\t\x1b[1;33m%s\x1b[0m\n", dir.d_name );
                // printf( "<dir>\t%s\n", dir.d_name );
                break;
                
            default: break;
        }

    }

    close(dir_fd);
    return 0;    
}