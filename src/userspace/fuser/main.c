#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/ptrace.h>
#include <dirent.h>
#include <string.h>

enum fuse_opcode {

    FUSE_LOOKUP = 1,
    FUSE_OPEN = 14,
    FUSE_READ = 15,
    FUSE_READDIR = 28,
};

struct fuse_req {
    unsigned long opcode;   // e.g. FUSE_LOOKUP, FUSE_READ
    unsigned long long unique;   // request ID
    unsigned long long nodeid;   // inode id
    unsigned long long offset;   // read offset, etc.
    unsigned long len;      // size of payload
    // payload follows
};

typedef struct {
    unsigned long len;
    signed long error;       // <0 means errno
    unsigned long long unique;     // must match request
    // payload follows: for LOOKUP, node attributes; for READ, file data
} fuse_reply_t;



int main(void){
    
    int err;
   int fusefd = open("/dev/fuse", O_RDWR);
   if(fusefd < 0){
        perror("/dev/fuse");
        exit(1);
   }

   char option_string[7] = "fd=000";
   sprintf(option_string, "fd=%u", fusefd);

   err = mount("fuse", "/mnt", "fuse", 0, option_string);
   if(err < 0){
        perror("failed to mount");
        exit(1);
   }



   char *directory_entries[] = {
        "rreadme.pls",
        "dentermepls",
        "bhunk",    
        "ccunt",
        "bUGUR",
        "cnigger",
   };


   
   unsigned char mbuff[512];
   struct fuse_req* req = (struct fuse_req*)mbuff;
    while( read(fusefd, req, 512) > 0 ){

        fuse_reply_t* rep = (void*)mbuff;
        rep->unique = req->unique;

        switch(req->opcode){
            case FUSE_READDIR:{
                if(req->offset < (sizeof(directory_entries) / sizeof(*directory_entries)) ){
                    
                    printf("[FUSER_READDIR] index:%u\n", req->offset);
                    write(fusefd, directory_entries[req->offset], strlen(directory_entries[req->offset]));
                }
                else{
                    write(fusefd, "!", 1);
                }
            }
            break;

            case FUSE_LOOKUP:{
                int i = 0;
                for(i = 0; i < (sizeof(directory_entries) / sizeof(*directory_entries)); ++i){
                    if(!memcmp(&req[1], &directory_entries[i][1], req->len)){

                        write(fusefd, directory_entries[i], strlen(directory_entries[i]));
                        i = -1;
                        break;
                    }
                }
                
                if(i > 0){

                    write(fusefd, "!", 1);
                }
                
            }
            break;

            default:
                write(fusefd, "!", 1);
            break;
        }
        
    }  

}