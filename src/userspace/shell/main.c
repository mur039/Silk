
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int fileno_stdout = 0;
int fileno_stderr = 0;
int fileno_stdin = 0;




struct dirent {
    uint32_t d_ino;
    uint32_t d_off;      
    unsigned short d_reclen;    /* Length of this record */
    int  d_type;      /* Type of file; not supported*/
    char d_name[256]; /* Null-terminated filename */
};

struct dirent readdir(int fd){
    //assuming given fd points to a dir in the fd table
    struct  dirent ret;
    int read_s = read(fd, &ret, sizeof(struct dirent));

    if(read_s == 0){ //eof
        ret.d_type = -1;
        return ret;
    }

    return ret;
}



typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

extern int open(const char * s, int mode);
extern size_t write(int fd, const void *buf, size_t count);


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;


// typedef struct
// {
//     unsigned char blue;
//     unsigned char green;
//     unsigned char red;
//     unsigned char alpha;
// } pixel_t;


int puts(const char * dst){
    write( fileno_stdout, dst, strlen(dst));
    return 0;
}

int putchar(int c){
    return write(fileno_stdout, &c, 1);
}

int getchar(){
    uint8_t ch = 0;
    int ret = read(fileno_stdin, &ch, 1);
    return ret ?  ch : -1;
}

//both arguments are NULL terminated
int is_word_in(const char * word, char ** list){
    
   if(word == NULL || list == NULL) return -1; //obv

   for(int i = 0;list[i] != NULL; ++i){
        if(!memcmp(word, list[i], strlen(list[i]))) {return i;}
   }
   return -1;
}

int is_word_in_n(char * word, size_t size, const char ** list){
   if(word == NULL || list == NULL) return -1; //obvs
   
   for(int i = 0;list[i] != NULL; ++i){
        if(!memcmp(word, list[i], size) ){
            return i;
        }
   }
   return -1;
}


const char * title_artwork =
                                "                   _                    \n"
                                " _____            | |       _       _ _ \n"
                                "|     |_ _ ___ ___|_|   ___| |_ ___| | |\n"
                                "| | | | | |  _| . |    |_ -|   | -_| | |\n"
                                "|_|_|_|___|_| |___|    |___|_|_|___|_|_|\n"
                                "\n";


#define COMMAND_BUFFER_MAXSIZE 36
int command_buffer_index = 0;
char command_buffer[COMMAND_BUFFER_MAXSIZE];


int shouldRun = 1;

enum command_ids {
    EXIT = 0,
    ECHO,
    EXEC,
    HELP,
    LS,
    CLEAR,
    SET
   
};

const char *commands[] = {
    [EXIT] = "exit",
    [ECHO] = "echo",
    [EXEC] = "exec",
    [HELP] = "help",
    [LS] = "ls",
    [CLEAR] = "clear",
    [SET] = "set",    


    
    NULL

};


typedef struct{
    char * src;
    size_t size;
} string_t;

//i have some internal variables that is settable by set VARIABLE_NAME VALUE

enum internal_var_enum{
    INTERNAL_VAR_DEBUG = 0
};

const char *internal_var_str[] = {
    [INTERNAL_VAR_DEBUG] = "debug",
    NULL
};


int internal_variables[] = {
    [INTERNAL_VAR_DEBUG] = 0
};


int is_delimeter(const char c){
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0');
}

const int max_words = 5;
int execute_command(){
    string_t words[max_words];
    for(int i = 0; i < max_words ; ++i)
    {
        words[i].src = NULL;
        words[i].size = 0;
    }

    int state = 0; //s0 not_in_word, s1 in_word
    char * start = command_buffer, * end = command_buffer;
    int i = 0;
    do{
        if(state == 0){
            if(is_delimeter(*start)){
                start++; //not in word
            }
            else{ //in word
                state = 1;
                end = start;
            }
        }
        else{ //state = 1
            if(is_delimeter(*end) || *end == '\0'){ //not in word

                state = 0;
                if(end > start){ //valid word
                    words[i].src = start;
                    words[i].size = (size_t)(end - start);
                    i++;
                    if( i >= max_words) break;
                }
               
                start = end;
            }
            else{
                end++;
            }
        }

    }
    while (*end != '\0');
    
    //parsed the things
    int index = is_word_in_n(words[0].src, words[0].size, commands);
    int result;

    switch (index)
    {
        
    case -1: //maybe i should execute a program here
        write(fileno_stdout, words[0].src, words[0].size);
        puts(" : command not found"); 
        break;

    case EXIT: //exit well fuck it
        puts("exit\n");

        // if(!words[1].src){ //NULL
        //     exit(0);
        //     return -1;
        // }

        // words[1].src[ words[1].size ] = '\0';
        // exit( atoi(words[1].src ) );
        // return -1;
        exit(0);
        break;

    case ECHO: //echo
        if(words[1].src == NULL){
            putchar('\n');
            break;
        }
        puts(words[1].src);
        break;

    case LS:
        if(words[1].src == NULL){
            puts("Expected a path\n");
            return -1;
        }

        char path_[64];
        for (unsigned int i = 0; i < words[1].size; ){
            path_[i] = words[1].src[i];
            i++;
            path_[i] = '\0';

        }
        int dir_fd = open(path_, O_RDONLY);
        if(dir_fd == -1){
            puts("No such directory\n");
            return -1;
        }


        while(1){
            struct dirent dir;
            dir = readdir(dir_fd);
            if(dir.d_type == -1) break;

            int off = 255;
            for(; dir.d_name[off] != '/'; off-- );

            if(dir.d_type == DIRECTORY){ //again
                --off;
                for(; dir.d_name[off] != '/'; off-- );
            }
            
            off++;
            
            
            printf( "%c    %s\n", dir.d_type == DIRECTORY ? 'd' : '-', &dir.d_name[off] );
            
        }

        close(dir_fd);
        break;

    case EXEC: //exec

        if(words[1].src == NULL){
            puts("Expected a path\n");
            return -1;
        }

        char path[64];
        for (unsigned int i = 0; i < words[1].size; ){
            path[i] = words[1].src[i];
            i++;
            path[i] = '\0';

        }


        char argv1[32];
        memcpy(argv1, words[2].src, words[2].size);
        
        int pipe_fd[2];
        if(pipe(pipe_fd) == -1){
            puts("Failed to create a pipe\n");
            return;
        }

        int pid = fork();

        if(pid == -1){
            puts("Failed to fork");
            return;
        }


        if(pid != 0){
            //parent doesn't need to read from pipe so closer fd[0]
            
            puts("parent: i'm waiting");
            // char command[] = "echo selam\n";
            // while(1){
            
            //     for(int i = 0; command[i] != '\0'; ++i){
            //         write(pipe_fd[1], &command[i], 1);
            //     }   
            //     for(int i = 0; i < 0xfffff; ++i);
            // }

        }
        else{ //imma child bitch
            ////child doesn't need to write to pipe so closer fd[1]
            
            dup2( pipe_fd[0], FILENO_STDIN);

            uint32_t _table[3];
            _table[0] = (uint32_t)path;
            _table[1] = (uint32_t)argv1[0] == '\0' ? 0 : argv1;
            _table[2] = (uint32_t)NULL;


            const char **_argv = (const char **)&_table;

            result = execve(
                            path,
                            _argv
                            );

            if(result == -1){
                puts("Failed to execute :(\n");
                exit(0);
            }
        }
        break;

    case HELP: //help
        puts( title_artwork );
        puts("Muro's shell V1.0\n");
        puts("Built-in commands:\n");
        for(int i = 0; commands[i] != NULL; ++i){
            printf("\t%s\n", commands[i]);
        }
        
        break;

   
    case CLEAR: //clear
        puts("\x1b[H"); //set cursor to (0,0)
        puts("\x1b[J"); //clear from cursor to end of the screen
        break;


    // set DEBUG 1
    case SET:
        
        if(words[1].src == NULL){
            puts("Expected variable_name:");
            return -1;
        }
        
        puts(words[1].src);

            // int _index = is_word_in_n(words[1].src, words[1].size, internal_var_str);
            // if(index == -1){ 
            //     puts("Invalid interval variable\n");
            //     return -1;
            // }

            // puts(internal_var_str[index]);
        


        break;

    default:
        break;
    }
    
   return 0;
}


int main(int argc, char **argv){
   
    fileno_stdout = 0;
    fileno_stdin  = 1;

    int c = 0;
    shouldRun = 1;
    

    puts(title_artwork);
    puts("> ");
    
    
    while(shouldRun){
        
       c = getchar();
        if(c != -1){             //i succesfully read something
        
            switch (c)
            {
            case 3: //^C //perhaps lets change it
                memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
                command_buffer_index = 0;
                puts("\n> ");
                continue;
                break;

            case '\b': //remove character
                command_buffer[command_buffer_index] = '\0';
                command_buffer_index -= 1;
                if(command_buffer_index >= 0){
                    puts("\x1b[1D \x1b[1D"); 
                }
                break;
            
            case 9: //tab
                break;
            case 10:
            case 13: //process the inout buffer
                command_buffer[command_buffer_index++] = '\n';
                command_buffer[command_buffer_index] = '\0';

                if(command_buffer_index > 1){ //if user entered anything

                    puts("\n");
                    execute_command();
                    memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
                }


                command_buffer_index = 0;
                puts("\n> ");
                break;

            // case 61: //left arrow    these send typically 2 byte currently one and is also a ascii character kbd driver issur
            // case 62: //right arrow   these send typically 2 byte currently one and is also a ascii character kbd driver issur
            case 60: // up arrow key
            case 63: // left arrow key
                break;


            default: //add character
                command_buffer[command_buffer_index] = c;
                command_buffer_index += 1;
                putchar(c);  //echo back to the user        
                break;
            }
            
        }
    }
    
    return 0;
}