
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;



int puts(char * dst){
    while(*(dst) != '\0') write( FILENO_STDOUT, (dst++), 1);
    return 0;
}
int putchar(int c){
    return write(FILENO_STDOUT, &c, 1);
}


int main(int argc, char ** argv){



    if(argc < 2){
        printf("Usage : %s FILE_NAME, for help use -h\n", argv[0] );
        return 1;
    }

    const char * file_path = argv[1];
    
    int fd_file = open(file_path, O_RDONLY);
    if(fd_file == -1){
        puts("No such file or directory\n");
        return 1;
    }


    stat_t st;
    if (fstat(fd_file, &st) == -1){
        puts("Failed to fstat the file\n");
        return 1;
    }

    int file_type = st.st_mode >> 16;
    if(file_type == DIRECTORY){
        printf("\"%s\" is a directory\n", file_path);
        return 1;
    }

    int _byte = 0;
    int i = 0;
    while( read(fd_file, &_byte, 1) != 0){
        continue;
        printf("%x ", _byte);
        if( i > 0 && !(i % 16) ){
            putchar('\n');
        }
        i++;
        // putchar(_byte);
    }

    return 0;
}