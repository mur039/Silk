#include <acpi.h>
static int rstrncmp(char *str1, char *str2) {
    for(int i=0;i<8 ;i++) {
        if(*str1++!=*str2++)return 1;
    }
    return 0;
}


struct RSDP_t find_rsdt(){
    struct RSDP_t ret; //NULL
    char *mem = (char*)0;
    for(int i=0;i<0x100000;i+=16) { //scanning below 1M
        if(!rstrncmp(&mem[i],"RSD PTR ")){
            ret = *(struct RSDP_t *)&mem[i];
            break;
        }
    }

    return ret;
}

