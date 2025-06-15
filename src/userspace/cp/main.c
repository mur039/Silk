
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signals.h>



int puts(const char * dst){
    write( FILENO_STDOUT, dst, strlen(dst));
    return 0;
}

int putchar(int c){
    return write(FILENO_STDOUT, &c, 1);
}

int getchar(){
    uint8_t ch = 0;
    int ret = read(FILENO_STDIN, &ch, 1);
    return ret ?  ch : -1;
}


enum options{
    
    // OPTION_HELP = 0,
    // OPTION_FILESYSTEM_TYPE,

    OPTION_EOT
};




const char* single_dash_options[] = {
    
    // [OPTION_HELP] = "-h",
    // [OPTION_FILESYSTEM_TYPE] = "-t",

    [OPTION_EOT] = NULL
};

const char* double_dash_options[] = {
    
    // [OPTION_HELP] = "--help",
    // [OPTION_FILESYSTEM_TYPE] = "--types",

    [OPTION_EOT] = NULL
};

const int need_options_arg[] = {
    
    // [OPTION_HELP] = 0,
    // [OPTION_FILESYSTEM_TYPE] = 1,

    [OPTION_EOT] = -1
};



//both str and list must be null terminated
int is_string_in_string_list(const char* str, const char** list){

    if( !(str && list) ) //one of them is null
        return -1;
    
    for(int i = 0; list[i] ; ++i){

        //if we find it here we return it

        if(!memcmp(str, list[i], strlen(list[i]) )) //we find it
            return i;
    }       

    //otherwise
    return -1;
}


const char help_string[] = 
                            "Usage:\n"
                            "\tmount [options] <source> <directory>:\n"
                            "\nMount a filesystem\n"
                            "\nOptions:\n"
                            
                            "-t, --types        specify the filesystem type\n"
                            "-h, --help         display this help\n"
                            ;

void print_help_str(){
    puts(help_string);
}


int main(int argc, char **argv){
    

    int src_target_index = 0;
    char* src_target_table[2];
    const char *source = NULL, *target = NULL, *file_system_type = NULL;

    if(argc == 1){
        puts("Expected arguments, see --help");
        return 1;
    }

    
    for(int i = 1; i < argc; ++i){
        
        int index = -1;
        if( !memcmp( argv[i], "--", 2 ) ){
            
            index = is_string_in_string_list(argv[i], double_dash_options);
        }else if( !memcmp( argv[i], "-", 1 ) ){
            
            index = is_string_in_string_list(argv[i], single_dash_options);
        }
        else{ //probably source or target
            index = -2;

        }



        switch(index){

            case -2:
                if(src_target_index < 2)
                    src_target_table[src_target_index++] = argv[i];
                else{
                    puts("mount: Bad usage try 'cp --help'\n");
                    return 1;
                }
                break;

            case -1:
                printf("Unknown option \"%s\"\n", argv[i]);
                break;

            // case OPTION_HELP:
            //     print_help_str();
            //     return 0;

            // case OPTION_FILESYSTEM_TYPE:
            //     if(file_system_type){
            //         puts("can't specify filesystem type twice!\n");
            //         return 1;
            //     }
            //     i++;
            //     file_system_type = argv[i];

            default:break;

        }
    }

    source = src_target_table[0];
    target = src_target_table[1];


    if(!source){
        puts("cp: <source> isn't specified\n");
        return 1;
    }


    if(!target){
        puts("cp: <target> isn't specified\n");
        return 1;
    }


    int fd_src = open(source, O_RDONLY);
    if(fd_src == -1){
        
        printf("No such file: %s\n", source);
        return 1;
    }

    stat_t src_stat;
    int err = fstat(fd_src, &src_stat);
    if(err != 0){
        printf("fstat failed.\n");
        return 1;
    }

    int src_file_type = src_stat.st_mode >> 16;
    if(src_file_type != REGULAR_FILE){
        
        printf("source is not a regular file\n");
        return 1;
    }

    int fd_target = open(target, O_WRONLY | O_CREAT, 0644); // create target file
    if(fd_target == -1){
        puts("failed to create target file\n");
        return 1;
    }

    size_t src_fsize = lseek(fd_src, 0, 2); //seekend
    lseek(fd_src, 0, 0); //seeset

    printf("source file size: %u\n", src_fsize);

    uint8_t byte;
    for(int i = 0; i < 4 ; ++i){
        read(fd_src, &byte, 1);
        printf("%x->%c\n", byte, byte);
        write(fd_target, &byte, 1);
    }


    while(read(fd_src, &byte, 1)){
    
        write(fd_target, &byte, 1);
    }

    return 0;
}