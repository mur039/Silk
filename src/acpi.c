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
    
    sum &= 0xff;
    return sum == 0;
}



void *find_sdt_by_signature(void *RootSDT, const char * signature){
    RSDT_t *rsdt = (RSDT_t *) RootSDT;
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;

    ACPISDTHeader_t **h = (ACPISDTHeader_t **)&rsdt->PointerToOtherSDT; //a table
    for (int i = 0; i < entries; i++)
    {
        if (!memcmp(h[i]->Signature, signature , 4))
            return (void *) h[i];
    }

    
    return NULL;
}


#include <fb.h>

typedef struct{
    ACPISDTHeader_t header;
    uint32_t local_apic_addr;
    uint32_t flags;
    uint8_t entries; //think of it as an array
} __attribute__((packed)) madt_t;

//entry type0
//entry type1
typedef struct{
    uint8_t io_apic_id;
    uint8_t reserved_0;
    uint32_t io_apic_addr;
    uint32_t global_sys_intr_base;
} __attribute((packed)) apic_type1_entry_t;


int acpi_parse_madt(ACPISDTHeader_t* table){

    //well first and foremost sanity check
    if(!table) 
        return 1;

    //validate the table by checksum
    if( !doSDTChecksum(table) ){
        return 2;
    }

    //then validate the name
    if(strncmp(table->Signature, "APIC", 4)){
        return 3;
    }

    //well it is correct so
    fb_console_printf("length of the madt table: %u\n", table->Length);
    
    size_t data_length = table->Length - sizeof(ACPISDTHeader_t);

    madt_t* madtable = (madt_t*)table;
    uint32_t* local_apic_addr = (uint32_t*)madtable->local_apic_addr;
    uint32_t flags = madtable->flags;
    
    fb_console_printf("local apic address: %x, flags: %x\n", local_apic_addr, flags);
    
    data_length -= 2*4;
    uint8_t* madt_entries = &madtable->entries;


    for(size_t i = 0; i < data_length; ){
        uint8_t entry = madt_entries[i];
        uint8_t record_len = madt_entries[i + 1];

        fb_console_printf("entry type:%u length:%u\n", entry, record_len);
        switch(entry){
            case 0:
                fb_console_printf("acip:Processor local APIC:\n");
                uint8_t acpi_proc_id, apic_id;
                uint32_t flags;
                acpi_proc_id = madt_entries[i + 2];
                apic_id = madt_entries[i + 3];
                flags = *(uint32_t*)(&madt_entries[i + 4]);
                fb_console_printf(
                                    "\tACPI processor ID: %u\n"
                                    "\tAPIC ID:           %u\n"
                                    "\tFlags:             %x\n",
                                    acpi_proc_id,
                                    apic_id,
                                    flags
                                    );
                break;

            case 1:
                fb_console_printf("acip: I/O APIC:\n");
                uint8_t io_apic_id = madt_entries[i + 2];
                uint32_t io_apic_addr = *(uint32_t*)&madt_entries[i + 4];
                uint32_t global_sys_intr_base = *(uint32_t*)&madt_entries[i + 4];;
                
                fb_console_printf(
                                    "\tI/O APIC ID:                  %u\n"
                                    "\tI/O APIC Address:             %x\n"
                                    "\tGlobal System Interrupt Base: %x\n"
                                    ,
                                    io_apic_id,
                                    io_apic_addr,
                                    global_sys_intr_base
                                    );
                break;

            case 2:
                fb_console_printf("acip: I/O APIC interrupt source override:\n");
                uint8_t bus_source = madt_entries[i + 2];
                uint8_t irq_source = madt_entries[i + 3];
                uint32_t global_sys_intr = *(uint32_t*)&madt_entries[ i + 4];
                uint16_t _flags = *(uint16_t*)&madt_entries[ i + 8];
                fb_console_printf(
                                    "\tBus Source: %u\n"
                                    "\tIRQ Source: %u\n"
                                    "\tGlobal System Interrupt: %x\n"
                                    "\tFlags: %x\n",
                                    bus_source, irq_source, global_sys_intr, _flags
                );
                break;  

            case 3:
                break;

            case 4:
                break;

            case 5:
                break;

            case 9:
                break;

            default: fb_console_put("what the fuck?\n");break;
        }

        i += record_len;
    }

    return 0;

}
