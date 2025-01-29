
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

int puts(char * dst){
    while(*(dst) != '\0') write( FILENO_STDOUT, (dst++), 1);
    return 0;
}
int putchar(int c){
    return write(FILENO_STDOUT, &c, 1);
}



enum options{
    OPTION_HELP = 0,
    OPTION_FILE_LENGTH,
    OPTION_FILE_OFFSET,

    OPTION_EOT
};



const char* single_dash_options[] = {
    
    [OPTION_HELP] = "-h",
    [OPTION_FILE_LENGTH] = "-l",
    [OPTION_FILE_OFFSET] = "-o",

    [OPTION_EOT] = NULL
};

const char* double_dash_options[] = {
    
    [OPTION_HELP] = "--help",
    [OPTION_FILE_LENGTH] = "--length",
    [OPTION_FILE_OFFSET] = "--offset",

    [OPTION_EOT] = NULL
};


//how many arguments
const int need_options_arg[] = {
    
    [OPTION_HELP] = 0,
    [OPTION_FILE_LENGTH] = 1,
    [OPTION_FILE_OFFSET] = 1,


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
                            "\nxxd [options] <source> n"
                            "\nOptions:\n"
                            
                            "-l, --length       specify length of the file\n"
                            "-o, --offset       specify read offset of the file\n"
                            "-h, --help         display this help\n"
                            ;

void print_help_str(){
    puts(help_string);
}



int main(int argc, char ** argv){



    int target_index = 0;
    char* src_target_table[1];
    const char *source = NULL, *len = NULL, *offset = NULL;

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
        else{ //probably  target
            index = -2;

        }



        switch(index){

            case -2:
                if(target_index < 1)
                    src_target_table[target_index++] = argv[i];
                else{
                    puts("xxd: Bad usage only specify one file at a time'\n");
                    return 1;
                }
                break;

            case -1:
                printf("Unknown option \"%s\"\n", argv[i]);
                break;

            case OPTION_HELP:
                print_help_str();
                return 0;

            case OPTION_FILE_LENGTH:
                if(len){
                    puts("can't specify length twice!\n");
                    return 1;
                }
                i++;
                len = argv[i];
                break;
            
            case OPTION_FILE_OFFSET:
                if(offset){
                    puts("can't specify offset twice!\n");
                    return 1;
                }
                i++;
                offset = argv[i];
                break;
            default:break;

        }
    }





    source = src_target_table[0];



    if(!source){
        puts("source isn't specified\n");
        return 1;
    }

    const char * file_path = source;
    
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

    if(offset){
        size_t offset = atoi(offset);
        lseek(fd_file, offset, 0); //sekkset
    }

    int length = -1;
    if(len){
        length = atoi(len);
    }

    
    // for(int j = 0; length == -1 || j < length  ; ++j){


        
        uint8_t line_buffer[16];
        memset(line_buffer, 0, 16);
        int repcount = 0;
        while( 1 ){
            int count = read(fd_file, line_buffer, 16);
            if(!count) break; //oef found

            for(int i = 0; i < count; ++i){
                printf("%x%x ", (line_buffer[i] >> 4) & 0xf, line_buffer[i] & 0xf );
            }
            
            //padding
            for(int i = 0; i < (16 - count); ++i){
                printf("  ");
            }

            //printing text section
            
            puts("| ");
            for(int i = 0; i < count; ++i){
                printf("%c", line_buffer[i] > 32 ? line_buffer[i] : '.' );
            }
            putchar('\n');

            memset(line_buffer, 0, 16);



            if(length != -1 && repcount >= length){
                break;
            }
            repcount++;
        }

        // break;
    // }

    return 0;
}