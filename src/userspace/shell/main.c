
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signals.h>

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


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;


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


int child_pid = -1;
int child_write_fd = -1;
int shell_state = 0;
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

void colour_test(){
    //for now only 8 colours
    for(int count = 0; count < 2; ++count){

        for(int i = 0; i < 8; ++i){
            printf("\x1b[1;%um", 30 + i);
            putchar(219);putchar(219);putchar(219);
        }
        putchar('\n');
    }

    printf("\x1b[0m\n");

}


int shouldRun = 1;
char* prompt_buf = NULL;
enum command_ids {
    EXIT = 0,
    ECHO,
    EXEC,
    HELP,
    LS,
    CLEAR,
    KILL,
    CD,
    PWD,
    USAGE
   
};

const char *commands[] = {
     [EXIT] = "exit",
     [ECHO] = "echo",
     [EXEC] = "exec",
     [HELP] = "help",
       [LS] = "ls",
    [CLEAR] = "clear",
      [KILL] = "kill",    
       [CD] = "cd",    
      [PWD] = "pwd",    
      [USAGE] = "usage",    


    
    NULL

};




typedef struct{
    char * src;
    size_t size;
} string_t;


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
        //yes let's try
        //reduce, reuse
        //see if we entered /bin/program



        
        write(FILENO_STDOUT, words[0].src, words[0].size);
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
        ;

        char path_[64];

        if(words[1].src == NULL){ // just ls so list current directory
            getcwd(path_, 64);
        }
        else{
            
            for (unsigned int i = 0; i < words[1].size; ){
                path_[i] = words[1].src[i];
                i++;
                path_[i] = '\0';

            }
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
            
    
    
    
    
    
    
    
    // printf( "%c    %s\n", dir.d_type == DIRECTORY ? 'd' : '-', &dir.d_name[off] );
    
            switch (dir.d_type){
                case REGULAR_FILE:
                case LINK_FILE:
                case CHARACTER_SPECIAL_FILE:
                case BLOCK_SPECIAL_FILE:
                case FIFO_SPECIAL_FILE:
                    printf( "<--->\t%s\n", &dir.d_name[off] );
                    break;

                case DIRECTORY:
                    printf( "<dir>\t\x1b[1;33m%s\x1b[0m\n", &dir.d_name[off] );
                    break;
                    

                default:
                break;
            }

            
            
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
            // close(pipe_fd[0]);
        
            // wait4(-1, NULL, 0, NULL); //wait for the child
            shell_state = 1;
            child_write_fd = pipe_fd[1];
            child_pid = pid;
            //when child exits, we will close some things
            
            

        }
        else{ //imma child bitch
            ////child doesn't need to write to pipe so closer fd[1]
            // close(pipe_fd[1]);
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
        colour_test();
        puts("Muro's shell V1.0\n");
        puts("Built-in commands:\n");
        for(int i = 0; commands[i] != NULL; ++i){
            printf("\t%s\n", commands[i]);
        }
        
        break;

   
    case CLEAR: //clear
        puts("\x1b[H"); //set cursor to (0,0)
        puts("\x1b[0J"); //clear from cursor to end of the screen
        break;


    // set DEBUG 1
    case KILL:
        
        if(words[1].src == NULL){
            puts("Expected pid");
            return -1;
        }
        
        {


        char * tmp = malloc(words[1].size + 1);
        memcpy(tmp, words[1].src, words[1].size);
        unsigned int kpid = atoi(tmp);
        kill(kpid, SIGKILL);
        free(tmp);
        }

    
        break;

    case CD:
        
        if(words[1].src == NULL){ // "cd" defaults to "/"
            chdir("/");
            sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", "/");
            break;
        }

        char* tmp = malloc(words[1].size + 1);
        memcpy(tmp, words[1].src, words[1].size + 1);
        tmp[words[1].size] = '\0';

        chdir(tmp);
        sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", tmp);
        free(tmp);
        break;

    case PWD:
        ;
        char* buf = malloc(64);
        getcwd(buf, 64);
        puts(buf);
        free(buf);
        break;

    case USAGE:
        ;
        struct sysinfo info;
        sysinfo(&info);

        printf(
            "Used ram: %u%s\n"
            "Free ram:  %u%s\n"
            "Total ram: %u%s\n"
            "Number of processes: %u\n"
            ,
            (info.totalram - info.freeram)/1024 > 999 ? (info.totalram - info.freeram)/(1024 * 1024) : (info.totalram - info.freeram)/1024,    (info.totalram - info.freeram)/1024 > 999 ? "MB" : "KB",
            
             info.freeram/1024 > 999 ? info.freeram/(1024 * 1024) : info.freeram/1024,    info.freeram/1024 > 999 ? "MB" : "KB",
             info.totalram/1024 > 999 ? info.totalram/(1024 * 1024) : info.totalram/1024,  info.totalram/1024 > 999 ? "MB" : "KB",
            
            info.procs
            );
        break;

    default:
        break;


    }
    
   return 0;
}


int pass_key_to_child(c){
    switch(c){
        case 3: //ctrl + c
            
            kill(child_pid, SIGKILL);
            shell_state = 0;
            child_pid = -1;
            break;

        case 26: //ctrl+z
            kill(child_pid, SIGSTOP);
            shell_state = 0;
            break;
            
        default : 
            ;
            if( write(child_write_fd, &c, 1) == -1){
                puts("child somehow exited?\n");
                shell_state = 0;
                child_pid = 0;

            }
            break;
    }
}




int process_key(int c){

    // printf("received character : %u:%x:%c\n", c, c, c);
     switch (c) {
            case 3: //^C //perhaps lets change it
                memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
                command_buffer_index = 0;
                printf("\n%s ", prompt_buf);
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
                printf("\n%s ", prompt_buf);
                break;

            // case 61: //left arrow    these send typically 2 byte currently one and is also a ascii character kbd driver issur
            // case 62: //right arrow   these send typically 2 byte currently one and is also a ascii character kbd driver issur
              

            case 0xe0: //up
                putchar(219);
                break;
            
            // case 0xe3: //down
            //     puts("\x1b[1B");
            //     break;

            default: //add character
                command_buffer[command_buffer_index] = c;
                command_buffer_index += 1;
                putchar(c);  //echo back to the user  
                // printf("%c : %x\n", c, c);      
                break;
            }

}



int main(int argc, char **argv){
   
    int c = 0;
    shouldRun = 1;

    malloc_init();
    char cwd_buf[32];
    //discard the err
    getcwd(cwd_buf, 32);


    puts("\x1b[H"); //set cursor to (0,0)
    puts("\x1b[0J"); //clear from cursor to end of the screen

    prompt_buf = malloc(32);
    sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", cwd_buf);
    // printf("current working directory: %s\n", cwd_buf);
    
    puts(title_artwork);
    colour_test();  
    
    printf("%s ", prompt_buf);

    memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
    while(shouldRun){
        
       c = getchar();
        if(c != -1){             //i succesfully read something

            if(!shell_state){

                process_key(c);
            }
            else{

                pass_key_to_child(c);
            }
            
        }

    }
    
    return 0;
}