
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signals.h>
#include <dirent.h>


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


#define COMMAND_BUFFER_MAXSIZE 128
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
    USAGE,
    RM,
    MKDIR,

   
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
      [RM] = "rm",
      [MKDIR] = "mkdir",



    
    NULL

};




typedef struct{
    char * src;
    size_t size;
} string_t;


int is_delimeter(const char c){
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0');
}

const int max_words = 6;
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
        //see if we entered /bin/program but we need env and PATH



        
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
            memcpy(path_, "./", 3);
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

            uint32_t ncount;
            struct dirent dir;
            ncount = getdents(dir_fd, &dir, sizeof(struct dirent));

            if(ncount == -1 || ncount == 0) break;

    
            switch (dir.d_type){
                case FS_FILE:
                    printf( "<--->\t%s\n", dir.d_name );
                    break;

                case FS_SYMLINK:
                    printf( "<sym>\t\x1b[1;34m%s\x1b[0m\n", dir.d_name );
                    // printf( "<sym>\t%s\n", dir.d_name );
                    break;

                case FS_BLOCKDEVICE:
                case FS_CHARDEVICE:
                case FS_PIPE:
                    printf( "<dev>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                    // printf( "<dev>\t%s\x1b\n", dir.d_name );
                    break;

                case FS_DIRECTORY:
                    printf( "<dir>\t\x1b[1;33m%s\x1b[0m\n", dir.d_name );
                    // printf( "<dir>\t%s\n", dir.d_name );
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



            string_t * proc_argv_strings = &words[1]; // programname + arguments
            int proc_argc;
            for(proc_argc = 0; proc_argv_strings[proc_argc].src != NULL; ++proc_argc);

            //create argv table for new process
            char** proc_argv = malloc( (proc_argc + 1) * sizeof(char*) );
            memset(proc_argv, 0, (proc_argc + 1) * sizeof(char*));

            for(int i = 0; i < proc_argc; ++i){
                
                proc_argv[i] = malloc(proc_argv_strings[i].size + 1);
                memset(proc_argv[i], 0, proc_argv_strings[i].size + 1);
                memcpy(proc_argv[i], proc_argv_strings[i].src, proc_argv_strings[i].size);

            }

          
            result = execve(
                            path,
                            proc_argv
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
    
    case RM:
        if(words[1].src == NULL){
            puts("Expected filename");
            return -1;
        }

        {

            char * tmp = malloc(words[1].size + 1);
            memcpy(tmp, words[1].src, words[1].size);
            tmp[words[1].size] = 0;
        
            unlink(tmp);
            free(tmp);
        }
        
        break;

    case MKDIR:
        if(words[1].src == NULL){
            puts("Expected directory name");
            return -1;
        }

        {

            char * tmp = malloc(words[1].size + 1);
            memcpy(tmp, words[1].src, words[1].size);
            tmp[words[1].size] = 0;
        
            mkdir(tmp, 0644);
            // free(tmp);
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

        if(!chdir(tmp)){
            free(tmp);
            tmp = malloc(64);
            getcwd(tmp, 64);
            sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", tmp);
        }
        else{
            printf("No such directory\n");
        }
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

    prompt_buf = malloc(32);
    sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", cwd_buf);
    // printf("current working directory: %s\n", cwd_buf);
    
    puts(title_artwork);
    colour_test();  
    
    printf("%s ", prompt_buf);

    memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
    while(shouldRun){
        
        if(shell_state){
            int wstat;
            int err = wait4(-1, &wstat, 0x1, NULL); //WHONOHANG

            if(err){ //child exited
                int child_exit_code = wstat & 0xff;
                printf("\nchild exited with code: %u\n", child_exit_code);
                shell_state = 0;
                printf("%s ", prompt_buf);
            }
        }


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