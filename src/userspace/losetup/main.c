#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <silk/loop.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

    int main(int argc, char *argv[]){
        int loopctlfd, loopfd, backingfile;
        long devnr;
        char loopname[4096];
        
        if (argc != 2) {
            fprintf(stderr, "Usage: %s backing-file\n", argv[0]);
            return EXIT_FAILURE;
        }

        loopctlfd = open("/dev/loop-control", O_RDWR);
        if (loopctlfd == -1)
            errExit("open: /dev/loop-control");
        devnr = ioctl(loopctlfd, LOOP_CTL_GET_FREE, NULL);

        if (devnr == -1)
            errExit("ioctl-LOOP_CTL_GET_FREE");
        sprintf(loopname, "/dev/loop%u", devnr);
        printf("loopname = %s\n", loopname);
        loopfd = open(loopname, O_RDWR);

        if (loopfd == -1)
            errExit("open: loopname");
            
        backingfile = open(argv[1], O_RDWR);
        if (backingfile == -1)
            errExit("open: backing-file");

        if (ioctl(loopfd, LOOP_SET_FD, (void*)backingfile) == -1)
            errExit("ioctl-LOOP_SET_FD");

        exit(EXIT_SUCCESS);
    }