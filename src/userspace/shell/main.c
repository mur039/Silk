

#ifndef NULL
#define NULL (void*)0
#endif


int fileno_stdout = 0;
int fileno_stderr = 0;
int fileno_stdin = 0;

typedef unsigned int size_t;
extern int read(int fd);
extern int close(int fd);
extern int exit(int code);
extern int execve(const char * path, const char * argv[]);

typedef struct  {
        //    unsigned int   st_dev;      /* ID of device containing file */
        //    unsigned int   st_ino;      /* Inode number */
           unsigned int  st_mode;     /* File type and mode */
        //    unsigned int st_nlink;    /* Number of hard links */
           unsigned int   st_uid;      /* User ID of owner */
           unsigned int   st_gid;      /* Group ID of owner */

} stat_t;

typedef enum {
    REGULAR_FILE = 0,
    LINK_FILE,
    RESERVED_2,
    CHARACTER_SPECIAL_FILE,
    BLOCK_SPECIAL_FILE,
    DIRECTORY,
    FIFO_SPECIAL_FILE,
    RESERVED_7
} file_types_t;

extern int fstat(int fd, stat_t * stat);

typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

extern int open(const char * s, int mode);


int test_function(int c){
    int fd = open("/dev/console", O_RDONLY);
    fileno_stdout = fd;
    put_hex(c);
    fileno_stdout = -1;
    close(fd);
    return 0;
}

extern size_t write(int fd, const void *buf, size_t count);


typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;

extern long lseek(int fd, long offset, int whence);

typedef struct
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char alpha;
} pixel_t;


int getline(char * dst){
    int c = 0, i = 0;
    while(1){
        c = read(fileno_stdout);
        if(c == -1) continue;
        if(c == '\n'){
            dst[i] = '\0';
             return i;
             }

        dst[i] = c;
        i += 1;
    }
}

int log_err(char *dst){
    while(*(dst) != '\0') write( fileno_stderr, (dst++), 1);
    return 0;
}
int puts(char * dst){
    while(*(dst) != '\0') write( fileno_stdout, (dst++), 1);
    return 0;
}

int putchar(int c){
    return write(fileno_stdout, &c, 1);
}

int getchar(){
    return read(fileno_stdin);
}

void memcpy(void * dest, void * src, size_t size, size_t nmemb){

    char  * pdest = dest;
    char  * psrc = src;
    for(size_t i = 0; i < size*nmemb; ++i){
        pdest[i] = psrc[i];
    }

    return;
}
int memcmp( void *s1,  void *s2, size_t size){ //simple true false
    unsigned char *b1, *b2;
    b1 = (unsigned char*)s1;
    b2 = (unsigned char*)s2;

    for(size_t i = 0 ; i < size ; i++){
        if(b1[i] != b2[i]){
            return 0;
        }
    }
    return 1;
}

void memset(void * addr, int c, size_t n){
    c &= 0xff;
    char * phead = addr;
    for(size_t i = 0; i < n ; ++i){
        phead[i] = c;
    }
}
int strlen(char * s){
    int i;
    for(i = 0; s[i] != '\0'; ++i);
    return i;
}

int is_word_in(char * word, char ** list){
    /*
    both word and list are null terminated
    if word is in list, then its position will be returned
    otherwise or in any error -1
    */
   if(word == NULL || list == NULL) return -1; //obv

   
   for(int i = 0;list[i] != NULL; ++i){
        if(memcmp(word, list[i], strlen(list[i]))) {return i;}
   }
   return -1;
}

int is_word_in_n(char * word, size_t size, char ** list){
   if(word == NULL || list == NULL) return -1; //obvs
   
   for(int i = 0;list[i] != NULL; ++i){
        if(memcmp(word, list[i], size) ){
            return i;
        }
   }
   return -1;
}


const char hexadecimal_characters[17] = "0123456789abcdef";
void put_hex(unsigned int hex){
    int non_zero = 0;
    for (int i = 0; i < 8; i++)
    {
        int nibble = (hex >> (28 -(4*i))) & 0xF;
        if(non_zero == 0 && nibble != 0){
            non_zero = 1;
            }
        if(non_zero|| (i == 7 && non_zero == 0) ){
            write(fileno_stdout, &hexadecimal_characters[nibble], 1);
        }
    }
    
    return;
}


#define COMMAND_BUFFER_MAXSIZE 64
int command_buffer_index = 0;
char command_buffer[COMMAND_BUFFER_MAXSIZE];

int shouldRun = 1;
const char *commands[] = {
    "exit",
    "echo",
    "exec",
    "help",
    "ls",
    "clear",
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
        write(fileno_stdout, words[0].src, words[0].size);
        puts(" : command not found"); 
        break;
    case 0: //exit well fuck it

        // if(words[1].src == NULL){
        //     puts("Expected exit code\n");
        //     return -1;
        // }

        // //turn str to int
        // int exit_code = 0;
        // for(size_t i = 0; i < words[1].size; ++i){
        //     exit_code *= 10;
        //     exit_code += words[1].src[i] - '0';
        // }
        exit(0);
        break;

    case 1: //echo
         if(words[1].src == NULL){
            putchar('\n');
            break;
        }
        puts(words[1].src);
        break;
    case 2: //exec

        if(words[1].src == NULL){
            puts("Expected a path\n");
            return -1;
        }

        char path[64];
        for (unsigned int i = 0; i < words[1].size; ++i){
            path[i] = words[1].src[i];
        }


        char argv1[32];
        memcpy(argv1, words[2].src, 1, words[2].size);
        

        const char * args[] = { 
                            path,
                            argv1,
                            NULL
                            };

        for(int i = 0; args[i] != NULL; ++i){
            puts(args[i]);putchar('\n');
        }
        result = execve(
                        path,
                        args
                        );

        if(result == -1){
            puts("Failed to execute :(\n");
        }
        break;

    case 3: //help
        puts(               
                "                   _                    \n"
                " _____            | |       _       _ _ \n"
                "|     |_ _ ___ ___|_|   ___| |_ ___| | |\n"
                "| | | | | |  _| . |    |_ -|   | -_| | |\n"
                "|_|_|_|___|_| |___|    |___|_|_|___|_|_|\n"
                "\n"
            );
        puts("Muro's shell V1.0\n");
        puts("Built-in commands:\n");
        for(int i = 0; commands[i] != NULL; ++i){
            putchar('\t');puts(commands[i]);putchar('\n');
        }
        
        break;

   
    case 5: //clear
        puts("\x1b[H"); //set cursor to (0,0)
        puts("\x1b[J"); //clear from cursor to end of the screen
        break;
    default:
        break;
    }
   

   return 0;
}


int main(int argc, char **argv){
    
    int fd_com1 = open("/dev/com1", O_RDWR);
    if(fd_com1 == -1){
        return 1;
    }

    int fd_kbd = open("/dev/kbd", O_RDONLY);
    if(fd_kbd == -1){
        return 1;
    }

    int fd_console = open("/dev/console", O_WRONLY);
    if(fd_console == -1){
        return 1;
    }


    fileno_stdout = fd_console;
    fileno_stdin  = fd_kbd;

    int c = 0;
    shouldRun = 1;
    
    
    
    while(shouldRun){
        
       c = getchar();
        
        if(c != -1){             //i succesfully read something
            switch (c)
            {
            case 3: //^C
                shouldRun = 0;
                continue;
                break;

            case '\b': //remove character
                command_buffer[command_buffer_index] = '\0';
                command_buffer_index -= 1;
                if(command_buffer_index >= 0){
                    puts("\x1b[1D \x1b[1D"); 
                }
                break;
                
            case 10:
            case 13: //process the inout buffer
                command_buffer[command_buffer_index++] = '\n';
                command_buffer[command_buffer_index] = '\0';
                puts("\n");
                execute_command();
                memset(command_buffer, 0, COMMAND_BUFFER_MAXSIZE);
                command_buffer_index = 0;
                puts("\n");
                puts("> ");
                break;

            case 224: // up arrow key
            case 225: // left arrow key
                break;


            default: //add character
                command_buffer[command_buffer_index] = c;
                command_buffer_index += 1;
                putchar(c);  //echo back to the user
                // put_hex(c);
                break;
            }
            
        }
    }
    
    return 0;
}