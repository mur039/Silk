#include <acpi.h>


static int rstrncmp(char *str1, char *str2) {
    for(int i=0;i<8 ;i++) {
        if(*str1++!=*str2++)return 1;
    }
    return 0;
}

int rsdp_is_valid(rsdp_t rdsp){
    uint8_t * byte = (uint8_t *)&rdsp;
    uint32_t checksum;

    for(uint32_t i = 0; i < sizeof(rsdp_t); i++){
        checksum += byte[i];
    }

    return (checksum & 0xff) == 0x00;
}

rsdp_t find_rsdt(){
    struct RSDP_t ret = {.Checksum = 0 }; //NULL
    char *mem = (char*)0;
    for(int i=0;i<0x100000;i+=16) { //scanning below 1M
        if(!rstrncmp(&mem[i],"RSD PTR ")){
            ret = *(struct RSDP_t *)&mem[i];
            break;
        }
    }

    return ret;
}

