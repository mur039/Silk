#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>
#include <string.h>


int get_pid_ttynr(pid_t pid){
    char lbuff[48];
    sprintf(lbuff, "/proc/%u/stat", pid);
    int mystat = open(lbuff, O_RDONLY);
    if(mystat < 0){
        return -1;
    }


    read(mystat, lbuff, 48);
    
    char* head = lbuff;
    for(int i = 0; i < 6; ){
        if(*head == ' '){
            i++;
        }
        head++;
    }

    char* tail = head;
    while( *tail != '\0' && isdigit(*(tail))){
        tail++;
    }
    memset(lbuff, 0, (size_t)(head-lbuff));
    memcpy(lbuff, head, (size_t)(tail - head));

    int ttynr = atoi(lbuff);
    return ttynr;
}

char* get_cmd(pid_t pid){
    static char lbuff[48];
    sprintf(lbuff, "/proc/%u/stat", pid);
    int mystat = open(lbuff, O_RDONLY);
    if(mystat < 0){
        return NULL;
    }


    read(mystat, lbuff, 48);
    
    char* head = lbuff;
    for(int i = 0;; ){
        if(*head == ' '){
            i++;
        }

        if(*head == '('){
            break;
        }
        head++;
    }

    head++; //consume (
    char* tail = head;
    while( *tail != '\0' && *tail != ')' ){
        tail++;
    }
    
    char tmp[48];
    memset(tmp, 0, 48);
    memcpy(tmp, head, (size_t)(tail - head));
    memset(lbuff, 0, 48);
    memcpy(lbuff, tmp, (size_t)(tail - head));

    return lbuff;
}


int main(){
    

    DIR* dirp = opendir("/proc/");
    if(!dirp){
        printf("failed to opendir /proc?\n");
        return 1;
    }
    struct dirent *ent = readdir(dirp);
    
    int mytty = get_pid_ttynr(getpid());
    while(ent != NULL){
        if(ent->d_name[0] >= '0' && ent->d_name[0] <= '9'){
            pid_t pid = atoi(ent->d_name);
            if(mytty == get_pid_ttynr(pid)){

                printf("%u  %u  %s \n", pid, get_pid_ttynr(pid), get_cmd(pid));
            }
            
        }
        ent = readdir(dirp);
    }
    closedir(dirp);


    return 0;    
}