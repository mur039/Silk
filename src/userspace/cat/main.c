
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

char line_buffer[64];
int main(int argc, char ** argv){

    


    if(argc < 2){
        
    int _byte = 0;
    while( read(FILENO_STDIN, &_byte, 1)){
        putchar(_byte);
    }

    }
    else{

        const char * file_path = argv[1];
        
        
        int fd_file = open(file_path, O_RDONLY);
        if(fd_file == -1){
            printf("No such file or directory");
            return 1;
        }

        stat_t st;
        if (fstat(fd_file, &st) == -1){
            printf("Failed to fstat the file\n");
            return 1;
        }

        int file_type = st.st_mode >> 16;
        if( S_ISDIR(file_type) ){
            printf("\"%s\" is a directory\n", file_path);
            return 1;
        }

        int _byte = 0;
        while( read(fd_file, &_byte, 1) > 0){

            if(putchar(_byte) < 0){
                perror("");
                exit(1);
            }
        }

    return 0;
    }
}