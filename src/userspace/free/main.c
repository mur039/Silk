#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdint.h>


enum byte_size {
    SIZE_BYTE = 0,
    SIZE_KBYTE,
    SIZE_MBYTE,
    SIZE_GBYTE,
};

int main(int argc, char* argv[]){
    
    struct sysinfo info;
    sysinfo(&info);
  
    size_t usedram = info.totalram - info.freeram;
    size_t totalram = info.totalram;

    printf("        total        used        free\n\n");
    printf("Mem      %u           %u          %u\n", totalram, usedram, info.freeram);
    
    return 0;
}