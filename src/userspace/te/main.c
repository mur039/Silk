
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signals.h>
#include <dirent.h>
#include <sys/socket.h>

typedef enum{
    SEEK_SET = 0,
    SEEK_CUR = 1, 
    SEEK_END = 2

} whence_t;





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




int main(int argc, char **argv){

    malloc_init();

    int err;
    int sockfd = socket(AF_UNIX, SOCK_RAW, 0);
    if(sockfd < 0){
        printf("[-] Failed to create socket\n");
        return 1;
    }

    socklen_t socklen = sizeof(struct sockaddr_un);
    struct sockaddr_un sockinfo;

    sockinfo.sun_family = AF_UNIX;
    sprintf(sockinfo.sun_path, "mds");
    
    err = bind(sockfd, &sockinfo, socklen);
    if(err < 0){
        printf("[-] Failed to bind the socket\n");
        return 1;
    }

    printf("[+] Created socket\n");

    err = listen(sockfd, 1);
    if(err < 0){
        printf("[-] Failed to listen\n");
        return 1;
    }
    printf("[+] Listening on the socket\n");

    pause(); 
    //we wait for any signals afterwards
    //then we accept an connection
    
    printf("[+] Accepting on the socket\n");
    err = accept(sockfd, NULL, NULL); //return info about client
    if(err < 0){
        printf("[-] Failed to accept?: %x\n", err);
        return 1;
    }

    int clientfd = err;
    printf("client fd that is :%u\n", clientfd);
    
    int ch = 0;
    while( (err = read(clientfd, &ch, 1)) > 0 ){

        printf(" %x\n", ch);
    }
    
    return 0;
}