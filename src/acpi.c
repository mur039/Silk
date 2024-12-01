#include <acpi.h>


static int rstrncmp(char *str1, char *str2) {
    for(int i=0;i<8 ;i++) {
        if(*str1++!=*str2++)return 1;
    }
    return 0;
}

int rsdp_is_valid(rsdp_t rdsp){
    uint8_t * byte = (uint8_t *)&rdsp;
    u8 checksum = 0;

    for(uint32_t i = 0; i < sizeof(rsdp_t); i++){
        checksum += byte[i];
    }

    return (checksum & 0xff) == 0x00;
}


rsdp_t find_rsdt(){
    struct RSDP_t ret = {.Checksum = 0 }; //NULL
    char *mem = (char*)0;
    for(int i=0;i<0x100000;i+=16) { //scanning below 1M
        if(!memcmp(&mem[i],"RSD PTR ", 8) ){
            ret = *(struct RSDP_t *)&mem[i];
            break;
        }
    }

    return ret;
}

int doSDTChecksum(ACPISDTHeader_t *tableHeader){
    unsigned char sum = 0;

    for (unsigned int i = 0; i < tableHeader->Length; i++)
    {
        sum += ((char *) tableHeader)[i];
    }

    return sum == 0;
}

void *find_sdt_by_signature(void *RootSDT, const char * signature){
    RSDT_t *rsdt = (RSDT_t *) RootSDT;
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;

    for (int i = 0; i < entries; i++)
    {
        ACPISDTHeader_t *h = (ACPISDTHeader_t *) rsdt->PointerToOtherSDT[i];
        if(!is_virtaddr_mapped(h)){
            map_virtaddr(h, h, PAGE_PRESENT | PAGE_READ_WRITE);
            flush_tlb();
        }

        if (!memcmp(h->Signature, signature , 4))
            return (void *) h;
    }

    // No FACP found
    return NULL;
}
