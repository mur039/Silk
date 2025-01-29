
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



typedef struct listnode{
    struct listnode* next;
    struct listnode* prev;
    void* val;

} listnode_t;


typedef struct list{
    struct listnode* head;
    struct listnode* tail;
    size_t size;

} list_t;



list_t create_list(){
    list_t ret;
    ret.head = NULL;
    ret.tail = NULL;
    ret.size = 0;
    return ret;
}


#define foreach(node, list)  for(listnode_t* node = list->head; node ; node = node->next)

listnode_t* insert_tail_list(list_t *list, void* val){

    listnode_t* node = calloc(1, sizeof(listnode_t));
    node->val = val;
    node->next = NULL;

    if(list->tail){
            
        node->prev = list->tail;

        list->tail->next = node;
        list->tail = node;
        list->size++;
        return node;
    }

    else{ //probably means list has no elements
        
        
        node->prev = NULL;
        list->head = node;
        list->tail = list->head;
        list->size++;
        return node;
    }


}


listnode_t* insert_head_list(list_t *list, void* val){

    listnode_t* node = calloc(1, sizeof(listnode_t));
    node->val = val;
    

    if(list->head){
        
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
        list->size++;
    }
    else{ //no members in list 
        
        node->prev = NULL;
        node->next = NULL;
        list->head = node;
        list->tail = list->head;
        list->size++;
        return node;
    }


}




int process_key(int c){

    //YES its gonna be like vi
    // printf("received character : %u:%x:%c\n", c, c, c);
     switch (c) {
            // case 3: //^C //perhaps lets change it
            //     break;

            // case '\b': //remove character
            //     break;
            
            // case 9: //tab
            //     break;
            // case 10:
            // case 13: //process the inout buffer
            
            // case 0x1b: //esc
            //     break;

                

            default: //add character
                putchar(c);  //echo back to the user  
                break;
            }

}




int shouldRun = 1;

int main(int argc, char **argv){


    // //parsing the arguments
    if(argc == 1){
        puts("expected a file\n");
        return 1;
    }


    malloc_init();

    char filepath[256];
    memcpy(filepath, argv[1], strlen(argv[1]) );

    

    list_t line_list = create_list();

    int file_fd = open(filepath, O_RDWR);

    if(file_fd == -1){
        file_fd = open(filepath, O_RDWR | O_CREAT, 0644);
        if(file_fd == -1){
            puts("Failed to create file.\n");
            return 1;
        }
    }
    else{
        //read file into the line list
        size_t file_size  = lseek(file_fd, 0, SEEK_END);
        printf("->size : %u\n", file_size);
        uint8_t* filebuf = malloc(file_size);

        lseek(file_fd, 0, SEEK_SET);
        read(file_fd, filebuf, file_size);

        
        uint8_t* prev = filebuf;
        uint8_t* head = filebuf;

        for(size_t i = 0; i < file_size + 1; ++i){

            if(i == file_size){
                size_t length = ((size_t)head) - ((size_t)prev);
                uint8_t* line = malloc( length);
                memcpy(line, prev, length);
                insert_tail_list(&line_list, line);
            }
            if(*head == '\n'){
                
                size_t length = ((size_t)head) - ((size_t)prev);
                uint8_t* line = malloc( length );
                memset(line, 0, length);
                memcpy(line, prev, length );
                insert_tail_list(&line_list, line);
                
                prev = head + 1;
            }
            head++;
        }   

        free(filebuf);
    }


    for(listnode_t* line = line_list.head; line ; line = line->next){
        printf("-> %s\n", line->val);
    }


    // while(shouldRun){
        
    //    char c = getchar();
    //     if(c != -1){             //i succesfully read something

    //         process_key(c);            
    //     }

    // }
    
    return 0;
}