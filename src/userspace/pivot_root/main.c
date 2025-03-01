
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
    OPTION_EOT
};




const char* single_dash_options[] = {
    
    [OPTION_HELP] = "-h",

    [OPTION_EOT] = NULL
};

const char* double_dash_options[] = {
    
    [OPTION_HELP] = "--help",

    [OPTION_EOT] = NULL
};

const int need_options_arg[] = {
    
    [OPTION_HELP] = 0,
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
                            "\tpivot_root <source> <directory>:\n"
                            ;

void print_help_str(){
    puts(help_string);
}


int main(int argc, char **argv){
    

    int src_target_index = 0;
    char* src_target_table[2];
    const char *source = NULL, *target = NULL;

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



    // printf("Well well well, %s->%s : %s\n", source, target, file_system_type);
    

    return 0;
}