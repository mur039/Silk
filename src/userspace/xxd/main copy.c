
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
    OPTION_HELP = 0,
    OPTION_FILESYSTEM_TYPE,

    OPTION_EOT
};




const char* single_dash_options[] = {
    
    [OPTION_HELP] = "-h",
    [OPTION_FILESYSTEM_TYPE] = "-t",

    [OPTION_EOT] = NULL
};

const char* double_dash_options[] = {
    
    [OPTION_HELP] = "--help",
    [OPTION_FILESYSTEM_TYPE] = "--types",

    [OPTION_EOT] = NULL
};

const int need_options_arg[] = {
    
    [OPTION_HELP] = 0,
    [OPTION_FILESYSTEM_TYPE] = 1,

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
                    puts("mount: Bad usage try 'mount --help'\n");
                    return 1;
                }
                break;

            case -1:
                printf("Unknown option \"%s\"\n", argv[i]);
                break;

            case OPTION_HELP:
                print_help_str();
                return 0;

            case OPTION_FILESYSTEM_TYPE:
                if(file_system_type){
                    puts("can't specify filesystem type twice!\n");
                    return 1;
                }
                i++;
                file_system_type = argv[i];

            default:break;

        }
    }

    source = src_target_table[0];
    target = src_target_table[1];


    if(!source){
        puts("mount: <source> isn't specified\n");
        return 1;
    }


    if(!target){
        puts("mount: <target> isn't specified\n");
        return 1;
    }


    if(!file_system_type){
        puts("mount: filesystem isn't specified\n");
        return 1;
    }


    // printf("Well well well, %s->%s : %s\n", source, target, file_system_type);
    int err_code = mount(source, target, file_system_type, 0, NULL);
    
    if(err_code < 0){ //error code
        printf("mount: failed to mount \"%s\" to \"%s\"\n", source, target);
        return 1;
    }


    return 0;
}