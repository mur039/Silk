
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


void xxd_main(int fd, int length);

int main(int argc, char ** argv){



    int target_index = 0;
    char* src_target_table[1];
    const char *source = NULL, *len = NULL, *offset = NULL;

    if(argc == 1){
        xxd_main(FILENO_STDIN, -1);
        return 0;
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
    const char * file_path = source;



    int fd_file;
    if(!source){
        fd_file = FILENO_STDIN;
    }
    else{

        fd_file = open(file_path, O_RDONLY);
        if(fd_file == -1){
            puts("No such file or directory\n");
            return 1;
        }
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
        size_t _offset = atoi(offset);
        lseek(fd_file, _offset, 0); //sekkset
    }

    int length = -1;
    if(len){
        length = atoi(len);
    }

    
    xxd_main(fd_file, length);
    

    
    return 0;
}

void xxd_main(int fd, int length){

    int fd_file = fd;
    int line_index = 0;
    uint32_t total_read = 0;
    uint8_t line_buffer[16];

    while(1){

        size_t tok = read(fd_file, &line_buffer[line_index], 1);
        total_read += tok;
        line_index += tok;

        //check if we read enough
        if(length != -1 && total_read >= length){
            tok = 0;
        }

        if(tok){
            
            if(line_index >= 16){
                
                line_index = 0;
                for(int i = 0; i < 16 ; ++i){
                    if(line_buffer[i] < 0x10 ){
                        printf("0%x ", line_buffer[i]);
                    }else{printf("%x ", line_buffer[i]);}
                }
                puts("  ");
                for(int i = 0; i < 16 ; ++i){
                    
                    char c = line_buffer[i];
                    if(c < 32){ putchar('.'); }
                    else{ putchar(c); }
                }
                putchar('\n');

            }

        }
        else{ 
            for(int i = 0; i < line_index; ++i){
				if(line_buffer[i] < 0x10 ){
                    printf("0%x ", line_buffer[i]);
                }else{
                    printf("%x ", line_buffer[i]);
                }
			}

			for(int i = line_index; i < 16; ++i){
				printf("   ");
			}


			printf("  ");
			for(int i = 0; i < line_index; ++i){
				unsigned char c = line_buffer[i];
					
				if( c > 32){
					putchar(c);
				}
				else{
					putchar('.');
				}
			}
			putchar('\n');
            break;
        }



    } 
       

}