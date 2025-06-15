
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signals.h>
#include <dirent.h>

#include <token.h>
#include <arena.h>


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

    if(ret < 0) return -1;
    else return ch;

    if(ret == -EAGAIN) return -1;
    else if(ret > 0) return ch;
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
            // puts(U"███");
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
    DISOWN,
    READ

   
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
    [USAGE] = "free",
    [RM] = "rm",
    [MKDIR] = "mkdir",
    [DISOWN] = "disown",
    [READ] = "read",
    NULL

};

typedef struct{
    char * src;
    size_t size;
} string_t;


int is_delimeter(const char c){
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0');
}


typedef struct listnode{
    void* val;
    struct listnode* next;

} listnode_t;

typedef struct{
    listnode_t* head;
    listnode_t* tail;
    size_t size;
} list_t;


int list_initalize(list_t* list){
    if(!list) return 0;

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return 1;
}

listnode_t* list_insert_end(list_t* list, void* val){
    
    listnode_t* node = malloc(sizeof(listnode_t));
    node->val = val;
    node->next = NULL;

    //empty list
    if(list->size == 0){
        
        list->head = node;
        list->tail = node;
        list->size = 1;
    }
    else{

        list->size++;
        list->tail->next = node;
        list->tail = node;
    }

    return node;
}

void* list_remove_head(list_t* list){
    
    if(!list->size) return NULL; 

    listnode_t* node = list->head;
    
    if(list->head == list->tail){ //one element that we just removed
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;
    }
    else{
        list->head = node->next;
        list->size--;
    }

    //free the node structure
    void* value = node->val;
    free(node);
    
    return value;
}




int list_directory(const char** argv){

    char path_[64];

    if(argv[0] == NULL){ // just ls so list current directory
        memcpy(path_, "./", 3);
    }
    else{
        
        memcpy(path_, argv[0], strlen(argv[0]) + 1);
    }


    int dir_fd = open(path_, O_RDONLY);
    if(dir_fd == -1){
        puts("No such directory\n");
        return 1;
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
                break;

            case FS_BLOCKDEVICE:
                printf( "<blk>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                break;

            case FS_CHARDEVICE:
                printf( "<chr>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                break;

            case FS_PIPE:
                printf( "<pip>\t\x1b[1;35m%s\x1b[0m\n", dir.d_name );
                break;
            
            case FS_DIRECTORY:
                printf( "<dir>\t\x1b[1;33m%s\x1b[0m\n", dir.d_name );
                // printf( "<dir>\t%s\n", dir.d_name );
                break;
                
            default: break;
        }

    }

    close(dir_fd);
    return 0;
}


int _kill(char** argv){
    if(!argv[0]){
        puts("Expected pid\n");
        return -1;
    }

    pid_t pid = atoi(argv[0]);
    int result = kill(pid, SIGKILL);
    if(result == -1){
        puts("No such pid\n");
        return -1;
    }
    else if(result == -2){
        printf("Insufficient permission to kill pid %u\n", pid);
        return -1;
    }

    return 0;

}

int _cd(char **argv){
    
    if(!argv[0]){
        chdir("/");
        sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", "/");
        return 0;
    }

    if(!chdir(argv[0])){ //no proplemo
        char _tmp[64];
        getcwd(_tmp, 64);
        sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", _tmp);
        return 0;
    }

    puts("No such directory\n");
    return 1;

}



static char unit_string[4][3] = {
    [0] = " B",
    [1] = "kB",
    [2] = "MB",
    [3] = "GB",
};

int _free(char **argv){
    struct sysinfo info;
    sysinfo(&info);

    char free[64], used[64], total[64];

    //crude size thingy
    
    int free_unit = 0;
    for(uint32_t i = info.freeram; i > 1024; i /= 1024 ){free_unit++;}

    if(free_unit == 0){ //Byte
        sprintf(free, "%u B", info.freeram); 
    }
    else if( free_unit == 1){ //kb darling
        sprintf(free, "%u," , info.freeram / 1024);
        sprintf(&free[strlen(free)], "%u kB" , ((info.freeram % 1024)*1000)/1024 );

    }
    else if( free_unit = 2){ //mB darling
        sprintf(free, "%u," , info.freeram / (1024 * 1024));
        sprintf(&free[strlen(free)], "%u MB" , ((info.freeram % (1024 * 1024))* (1000*1000) )/(1024*1024) );
    }




    uint32_t used_ram = info.totalram - info.freeram;
    int used_unit = 0;
    for(uint32_t i = used_ram; i > 1024; i /= 1024 ){used_unit++;}

    if(used_unit == 0){ //Byte
        sprintf(used, "%u B", used_ram); 
    }
    else if( used_unit == 1){ //kb darling
        sprintf(used, "%u," , used_ram / 1024);
        sprintf(&used[strlen(used)], "%u kB" , ((used_ram % 1024)*1000)/1024 );

    }
    else if( used_unit = 2){ //mB darling
        sprintf(used, "%u," , used_ram / (1024 * 1024));
        sprintf(&used[strlen(used)], "%u MB" , ((used_ram % (1024 * 1024))* (1000*1000) )/(1024*1024) );
    }

    


    int total_unit = 0;
    for(uint32_t i = info.totalram; i > 1024; i /= 1024 ){total_unit++;}

    if(total_unit == 0){ //Byte
        sprintf(total, "%u B", info.totalram); 
    }
    else if( total_unit == 1){ //kb darling
        sprintf(total, "%u," , info.totalram / 1024);
        sprintf(&total[strlen(total)], "%u kB" , ((info.totalram % 1024)*1000)/1024 );

    }
    else if( total_unit = 2){ //mB darling
        sprintf(total, "%u," , info.totalram / (1024 * 1024));
        sprintf(&total[strlen(total)], "%u MB" , ((info.totalram % (1024 * 1024))* (1000*1000) )/(1024*1024) );
    }



    printf(
        "Used  RAM: %s\n"
        "Free  RAM: %s\n"
        "Total RAM: %s\n",
        used,
        free,
        total
    );
    return 0;

}

int _pwd(char **argv){
    
    char buff[64];
    getcwd(buff, 64);
    puts(buff);
    return 0;
}



int _exec(char **argv){
    
    if(!argv[0]){
        puts("Expected executable path\n");
        return 1;
    }

    int pipefd[2]; //read:write
    if(pipe(pipefd) == -1){
        puts("Failed to create a pipe\n");
        return 1;
    }

    int pid = fork();
    if(pid == -1){
        puts("Failed to fork\n");
        return 1;
    }

    if(pid != 0){
        
        //parent will only write so there's no point on reading
        close(pipefd[0]);

        shell_state = 1;
        child_write_fd = pipefd[1];
        child_pid = pid;
        return 0;
    }
    else{

        close(pipefd[1]);    
        dup2( pipefd[0], FILENO_STDIN);
        //and child will only read so so there's no point writing back
        
        int result = execve(
                            argv[0],
                            argv
                            );
        
        if(result == -1){
            puts("Failed to execute\n");
            exit(1);
        }

    }


}


int _mkdir(char **argv){
    
    //for now only one
    if(!argv[0]){
        puts("Expected a folder name\n");
        return 1;
    }

    mkdir(argv[0], 0775);
    return 0;
}

int _rm(char **argv){
    
    //for now only one
    if(!argv[0]){
        puts("Expected a folder name\n");
        return 1;
    }
    char* path = argv[0];
    return unlink(path);
    return 1;
}


int _disown(char **argv){

    if(!argv[0]){
        puts("Expected executable path\n");
        return 1;
    }

    int pid = fork();
    if(pid == -1){
        puts("Failed to fork\n");
        return 1;
    }

    if(pid != 0){
        
        //parent will only write so there's no point on reading
        
        shell_state = 0;
        child_write_fd = -1;
        child_pid = -1;
        return 0;
    }
    else{

        int result = execve(
                            argv[0],
                            argv
                            );
        
        if(result == -1){
            puts("Failed to execute\n");
            exit(1);
        }

    }

    return 0;
}


int _read(char **argv){

    if(!argv[0]){
        puts("Expected file path\n");
        return 1;
    }

    
    int filefd = open(argv[0], O_RDONLY);
    if(filefd < 0){
        puts("Failed to open file for reading\n");
        return 1;
    }


    int ch;
    while( read(filefd, &ch, 1) > 0) putchar(ch); //read until eof or other error
    return 0;
}


int execute_command(char *command_buffer){
    
    printf("the buffer i received is: %s\n", command_buffer);
    list_t list;
    list_initalize(&list);

    Token tokens[17];
    size_t token_count = tokenize(command_buffer, tokens, 16);

    

    
    if(token_count == 0){
        return 0;
    }
    
    //well for now it'll work like single command no piping no redirection
    for(size_t i = 0; i <= token_count; ++i){
        Token *tok = &tokens[i];
        switch (tok->type){
        case TOKEN_WORD: 
            printf("%u -> { TOKEN_WORD: \"%s\"}\n", i, tok->value); 
            list_insert_end(&list, tok->value);
            break;
        default:
            i = token_count; //to kill the loop :/
            break;
        }
        
    }

    listnode_t* cnode = list.head;
    list.head = list.head->next;
    list.size--;

    char* command = cnode->val;
    free(cnode);
    
    char** argv = calloc( list.size + 1, sizeof(char*));
    char** h = argv;
    for(listnode_t* node = list.head; node         ; node = node->next){

        char* str = node->val;
        printf("-> %s \n", str);
        *h = str;
        h++;
    }


    int index = is_word_in(command, commands);
    switch(index){
        case -1:puts("No such command");break;
        case CLEAR: //clear
            puts("\x1b[H"); //set cursor to (0,0)
            puts("\x1b[0J"); //clear from cursor to end of the screen
        break;

        case EXIT: //exit well fuck it
            ;
            int exitcode = 0;
            if(argv[0]){ //if it exist

                //check whether it'a number
                exitcode = atoi(argv[0]);
            }

            printf("exit code: %u\n", exitcode);
            exit(exitcode); 
        break;


        case ECHO: //echo
            for(int i = 0; argv[i] != NULL; ++i){
                printf("%s ", argv[i]);
            }
            putchar('\n');

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

        case LS:    return list_directory(argv); break;
        case CD:    return            _cd(argv); break;
        case KILL:  return          _kill(argv); break;
        case USAGE: return          _free(argv); break;
        case PWD:   return           _pwd(argv); break;
        case EXEC:  return          _exec(argv); break;
        case MKDIR: return         _mkdir(argv); break;
        case RM:    return            _rm(argv); break;
        case DISOWN:return         _disown(argv);break;
        case READ:return             _read(argv);break;

        default:break;
    }






    

    free(argv);
    return 0;
}

int did_child_exited(){
    int err, wstat;
    err = wait4(-1, &wstat, WNOHANG, NULL); //WHONOHANG
    if(err != 0){
        
        int child_exit_code = wstat & 0xff;
        
        
        if( WTERMSIG(wstat) ){
            printf("\nChild killed with signal: %u\n", child_exit_code);
            close(child_write_fd);
            child_write_fd = -1;
            return 1;

        }
        else{

            printf("\nchild exited with code: %u\n", child_exit_code);
            return 1;
        }

    }

    return 0;
}

int pass_key_to_child(c){
    int err;

    switch(c){
        case 3: //ctrl + c
            
            kill(child_pid, SIGKILL);
            break;

        case 26: //ctrl+z
            kill(child_pid, SIGUSR1);
            break;
            
        default : 
            err = write(child_write_fd, &c, 1);
            if(!err){

                puts("child somehow exited?\n");
                close(child_write_fd);
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
                    execute_command(command_buffer);
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
    char pb[64];
    //discard the err
    getcwd(cwd_buf, 32);

    prompt_buf = pb;//malloc(32);
    sprintf(prompt_buf, "\x1b[1;32m[%s]>\x1b[0m", cwd_buf);
    // printf("current working directory: %s\n", cwd_buf);
    
    puts(title_artwork);
    colour_test();  
    
    printf("%s ", prompt_buf);


    memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
    while(shouldRun){
        
        c = getchar();
        
        if(shell_state && did_child_exited()){
            shell_state = 0;
            printf("\n%s ", prompt_buf);
        }

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