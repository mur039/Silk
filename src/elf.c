
#include <elf.h>
#include <process.h>
static const unsigned char ELF_MAGIC[4] = {0x7f, 'E', 'L', 'F'};

static int elf_is_valid(file_t * file){
    char * byte = (void *)&file->f_inode[1];
    for(int i = 0; i < 4; ++i){
        if(byte[i] != ELF_MAGIC[i]) { return 0;}
    }
    return 1;
}

void * elf_get_entry_address(file_t * file){
    if(elf_is_valid(file) == 0) { return NULL;} 
    Elf32_Ehdr * header = (void *)&file->f_inode[1];

    //see if it's an executable
    if(header->e_type != 2){
        uart_print(COM1, "It is not an executable\r\n");
        return NULL;
        }

    //see if it's for x86
    if(header->e_machine != 0x03){
        uart_print(COM1, "It is not x86\r\n");
        return NULL;
        }

    //see if current version?
    if(header->e_version != 1) {
        uart_print(COM1, "Current version is not 1\r\n");
        return NULL;
        }

    

    return (void *)header->e_entry;
}


int elf_get_filesz(file_t * file){
     if(elf_is_valid(file) == 0) { return NULL;} 
    Elf32_Ehdr * header = (void *)&file->f_inode[1];


    unsigned char * phead = (void *)header;
    phead += header->e_phoff;
    Elf32_Phdr * p_head = (void *)phead; 

    return p_head->filesz;
}

int elf_list_program_headers(file_t * file){
    //see if its valid
    if(elf_is_valid(file) == 0){
        return 0;
    }

    Elf32_Ehdr * header = (void *)&file->f_inode[1];
    unsigned char * phead = (void *)header;
    phead += header->e_phoff;
    Elf32_Phdr * p_head = (void *)phead; 

    for(int i = 0; i < header->e_phnum; ++i){
        
        uart_print(COM1, "type:%x offset:%x vaddr:%x paddr:%x filesize:%x memsz:%x\r\n",
        p_head[i].type,
        p_head[i].offset,
        p_head[i].vaddr,
        p_head[i].paddr,
        p_head[i].filesz,
        p_head[i].memsz
        );
    }

    return 1;
}

//gonna be called by schedular for TASK_CREATED 
//and just load the elf to the memory
void * elf_load(pcb_t * process){
    
    file_t init;    
    init.f_inode = tar_get_file(process->filename, O_RDONLY);
    if(init.f_inode == NULL){
        uart_print(COM1, "Failed to open %s\r\n", process->filename);
        return NULL;
    }
    init.f_mode = O_RDONLY; 
 

    uint8_t * program_entry;

    program_entry = (void*)elf_get_entry_address(&init);
    uart_print(COM1, "init's entry point : %x\r\n",  program_entry );
    if(program_entry == NULL){
        uart_print(COM1, "/bin/init is not a valid ELF executable\r\n");
        process->state = TASK_ZOMBIE;
        return NULL;
    }

    
    Elf32_Ehdr * header = (void *)&init.f_inode[1];
    unsigned char * phead = (void *)header;
    phead += header->e_phoff;
    Elf32_Phdr * p_head = (void *)phead; 

    if(p_head->type == 1){
        phead = (unsigned char *)header;
        phead += p_head->offset;
    }

    // pmm_print_usage();
    void * program_data;
    uart_print(COM1, "program filesz %x\r\n", p_head->filesz);
    program_data = kpalloc(1 + p_head->filesz/4096);// allocate_physical_page();
    //map_virtaddr(program_data, program_data, PAGE_PRESENT | PAGE_READ_WRITE);

    memset(program_data, 0, 4096);
    memcpy(program_data, phead, p_head->filesz);    
    uart_print(COM1, "program_data %x\r\n", program_data);
    //flush_tlb();
    
    //map_virtaddr(program_data, program_data, PAGE_PRESENT | PAGE_READ_WRITE | PAGE_USER_SUPERVISOR);
    return program_data;

}