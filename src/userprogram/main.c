

#ifndef NULL
#define NULL (void*)0
#endif

typedef unsigned int size_t;
extern int read(int fd);
extern int close(int fd);
extern int exit(int code);


extern size_t write(int fd, const void *buf, size_t count);
typedef enum{
    O_RDONLY = 0b001,
    O_WRONLY = 0b010, 
    O_RDWR   = 0b100

} file_flags_t;

extern int open(const char * s, int mode);

typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;

extern long lseek(int fd, long offset, int whence);
int fileno_stdout = 0;
int fileno_stderr = 0;

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

typedef unsigned int size_t;

#define COMMAND_BUFFER_MAXSIZE 64

int command_buffer_index = 0;
char command_buffer[COMMAND_BUFFER_MAXSIZE];

int memcmp( void *s1,  void *s2, size_t size){
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


int shouldRun = 1;
const char *commands[] = {
    "exit",
    "echo",
    NULL
};

int execute_command(){

    //parse the command buffer
    /*
        first argument is the program 
        valid commands:
            echo $argument
            exit $argument

    */

   int index_1 = 0, index_2 = 0;
   
   int pieces_index = 0;
   
   while(1){

    if(command_buffer[index_2] == ' '){
        // write(fileno_stdout, &(command_buffer[index_1]), index_2 - index_1);

        if(pieces_index++ == 0){ //thats command see if there's any
            if(memcmp(&(command_buffer[index_1]), "exit", 4)){
                exit(0x69);
            }
        }
       
        index_1 = ++index_2;

    }


    if(command_buffer[index_2] == '\0' ){

        if(pieces_index++ == 0){ //thats command see if there's any
            if(memcmp(&(command_buffer[index_1]), "exit", 4)){
                exit(0x69);
            }
        }
        // write(fileno_stdout, &(command_buffer[index_1]), index_2 - index_1);
        // putchar('\r\n');
        break;
    }
    index_2++;  
   }

    
   return 0;
}


int main(){

    
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
    fileno_stderr = fd_com1;

    int c = 0;
    shouldRun = 1;
    
    puts("> ");

    while(shouldRun){
       c = read(fd_kbd);

        if(c != -1){
            //i succesfully read something
            switch (c)
            {
            case 3: //^C
                shouldRun = 0;
                continue;
                break;

            case 10:
            case 13:
                command_buffer[command_buffer_index] = '\0';
                command_buffer_index = 0;
                puts("\r\n");
                execute_command();
                puts("> ");
                break;

            default:
                command_buffer[command_buffer_index] = c;
                command_buffer_index += 1;
                putchar(c);  //echo back to the user
                break;
            }
            
        }
    }
    
    return 0;
}