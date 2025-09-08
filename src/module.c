#include <module.h>
#include <sys.h>
#include <syscalls.h>
#include <pmm.h>
#include <elf.h>

struct module *modules = NULL; // linked list head

void *ksym_lookup(const char *name) {
    for (struct ksym *k = __ksymtab_start; k < __ksymtab_end; ++k)
        if (!strcmp(k->name, name)) return k->addr;
    return NULL;
}

static uint32_t round_up(uint32_t addr, uint32_t align) {
    return (align ? (addr + align - 1) & ~(align - 1) : addr);
}


void syscall_init_module(struct regs* r){
    
    void* module_image = (void*)r->ebx;
    unsigned long module_len = (unsigned long)r->ecx;
    const char* param_values = (const char*)r->edx;

    //check the addr
    if(!is_virtaddr_mapped(module_image)){
        r->eax = -EFAULT;
        return;
    }

    Elf32_Ehdr* ehdr = module_image;
    if(memcmp(ehdr->e_ident, "\x7f\x45\x4c\x46", 4) ){
        //invalid ELF ehdr
        r->eax = -ENOEXEC;
        return;    
    }
    
    if(ehdr->e_machine != EM_386 ){
        //not architecture compatible
        r->eax = -ENOEXEC;
        return;    
    }

    if(ehdr->e_type != ET_REL ){
        //not relocatable
        r->eax = -ENOEXEC;
        return;    
    }

    
    //copy the user buffer in to a kernel buffer
    uint8_t* elf_image = kcalloc(module_len, 1);
    if(!elf_image){
        r->eax = -ENOMEM;
        return;
    }
    memcpy(elf_image, module_image, module_len);
    

    ehdr = (void*)elf_image;
    Elf32_Shdr* shdr = (Elf32_Shdr*)((uint8_t*)ehdr + ehdr->e_shoff);

    

    //at this point the module looks loadable..
    struct module* module = kcalloc(sizeof(struct module), 1);

    Elf32_Shdr *shstr = &shdr[ehdr->e_shstrndx];
    const char *shstrtab = (const char *)elf_image + shstr->sh_offset;
     
    // calculate the size of writeable and non writable sections
    unsigned int writable_size = 0;
    unsigned int readonly_size = 0;
    unsigned int allocable_section_count = 0;
    for (int i=0; i<ehdr->e_shnum; i++) {
        
        if (shdr[i].sh_flags & SHF_ALLOC) {
            allocable_section_count++;
            if(shdr[i].sh_flags & SHF_WRITE) writable_size += shdr[i].sh_size;
            else readonly_size += shdr[i].sh_size;
        }
    }


    size_t writable_page_count = (writable_size / 4096) + (writable_size % 4096 != 0);
    size_t nonwritable_page_count = (readonly_size / 4096) + (readonly_size % 4096 != 0);
    
    //allocate them on the kernel
    uint8_t* ro = vmm_get_empty_kernel_page(writable_page_count + nonwritable_page_count, (void*)0xD0000000);
    uint8_t* wr = ro + (writable_page_count*4096);
    
    module->base = ro;
    module->size = (writable_page_count + nonwritable_page_count) * 4096;
    module->section_count = allocable_section_count;
    module->sections = kcalloc(allocable_section_count, sizeof(struct ksym));
     for(uint8_t* head = ro; head < (ro + (writable_page_count + nonwritable_page_count)*4096); head += 4096){
        map_virtaddr(head, allocate_physical_page(), PAGE_PRESENT | PAGE_READ_WRITE);
        memset(head, 0, 4096);
    }

    //copy it over
    uint8_t *wr_head = wr, *ro_head=ro;
    for (int i=0; i<ehdr->e_shnum; i++) {
        
        if ( (shdr[i].sh_flags & SHF_ALLOC) ) {
            
            if(shdr[i].sh_type == SHT_PROGBITS){

                if(shdr[i].sh_flags & SHF_WRITE){
                    wr_head = (void*)round_up((uint32_t)wr_head, shdr[i].sh_addralign);
                    shdr[i].sh_addr = (uint32_t)wr_head;
                    memcpy(wr_head, elf_image + shdr[i].sh_offset, shdr[i].sh_size);
                    wr_head += shdr[i].sh_size;
                }    
                else{
                    ro_head = (void*)round_up((uint32_t)ro_head, shdr[i].sh_addralign);
                    shdr[i].sh_addr = (uint32_t)ro_head;
                    memcpy(ro_head, elf_image + shdr[i].sh_offset, shdr[i].sh_size);
                    ro_head += shdr[i].sh_size;
                }
            }
            else if(shdr[i].sh_type == SHT_NOBITS){
                //though should be zeroed, i zeroed all the buffers...
                wr_head = (void*)round_up((uint32_t)wr_head, shdr[i].sh_addralign); //align it
                shdr[i].sh_addr = (uint32_t)wr_head;
                wr_head += shdr[i].sh_size;
            }
            
            

        }
    }
    

    // for (int i=0; i<ehdr->e_shnum; i++) {
        
    //     if ( (shdr[i].sh_flags & SHF_ALLOC) ) {
    //         const char *sec_name = shstrtab + shdr[i].sh_name;
    //         fb_console_printf("section %s : baseaddr : %x \n", sec_name, shdr[i].sh_addr);


    //     }
    // }
    

    //may be i should keerp track of the loaded sections...
    //perform relocations somehow           
    for (int i = 0; i < ehdr->e_shnum; i++) {
      
        if (shdr[i].sh_type == SHT_REL) {
        const char *sec_name = shstrtab + shdr[i].sh_name;
        
        // fb_console_printf("Section %s\r\n", sec_name);


        Elf32_Shdr *symtab_sec = &shdr[shdr[i].sh_link];   // associated symbol table
        Elf32_Sym *symtab = (Elf32_Sym *)(elf_image + symtab_sec->sh_offset);

        // get string table for symbol names
        Elf32_Shdr *strtab_sec = &shdr[symtab_sec->sh_link];
        const char *strtab = (const char *)(elf_image + strtab_sec->sh_offset);

        size_t rel_count = shdr[i].sh_size / shdr[i].sh_entsize;
        for (size_t j = 0; j < rel_count; j++) {
            Elf32_Rel *rel = (Elf32_Rel *)(elf_image + shdr[i].sh_offset + j * sizeof(Elf32_Rel));
            uint32_t sym_index = ELF32_R_SYM(rel->r_info);
            unsigned char type = ELF32_R_TYPE(rel->r_info);
            Elf32_Sym *sym = &symtab[sym_index];
            

            Elf32_Shdr *target = &shdr[shdr[i].sh_info];
            if(!target->sh_addr) 
                break;

            const char* symbolstr = strtab + sym->st_name; 
            uint32_t* P = (uint32_t*)(target->sh_addr + rel->r_offset);
            uint32_t S;
            uint32_t A;

            switch (type){
            case R_386_NONE: continue;break; //obv 
            
            case R_386_32:
                
                if(sym->st_shndx > 0){ //normal section
                
                    S = shdr[sym->st_shndx].sh_addr + sym->st_value;
                    A = *P;
                    *P = S + A;
                }
                else if(sym->st_shndx == SHN_ABS){ 
                    S = sym->st_value;
                    A = *P;
                    *P = S + A;
                }
                else if(sym->st_shndx == SHN_UNDEF){ //UNDEF //must loopkup
                    S = (uint32_t)ksym_lookup(symbolstr);
                    if(!S){
                        //deallocation neydi deallocation sevgi idi emek idi
                        fb_console_printf("Failed to find symbol %s\n", symbolstr);
                        r->eax = -ENOEXEC;
                        return;
                    }
                    A = *P;
                    *P = S + A;
                }
            break;

            case R_386_PC32:

                if(sym->st_shndx > 0){ //normal section
                    
                    S = shdr[sym->st_shndx].sh_addr + sym->st_value;
                    A = *P;
                    *P = S + A - (uint32_t)P;
                    
                }
                else if(sym->st_shndx == SHN_UNDEF){ //UNDEF //must loopkup
                    S = (uint32_t)ksym_lookup(symbolstr);
                    if(!S){
                        //deallocation neydi deallocation sevgi idi emek idi
                        fb_console_printf("Failed to find symbol %s\n", symbolstr);
                        r->eax = -ENOEXEC;
                        return;
                    }
                    A = *P;
                    *P = S + A - (uint32_t)P;
                }
                else if(sym->st_shndx == SHN_ABS){ 
                    S = sym->st_value;
                    A = *P;
                    *P = S + A - (uint32_t)P;
                }
                // fb_console_printf("\t R_386_PC32 -> shdx:%x %s\n", sym->st_shndx, symbolstr);
            break;

            case R_386_GOT32:
            case R_386_PLT32:
            case R_386_COPY:
            case R_386_GLOB_DAT:
            case R_386_JMP_SLOT:
            case R_386_RELATIVE:
            case R_386_GOTOFF:
            case R_386_BASE32:
            break;

            }
        }
      }
    }


    //set flags    
    for(size_t i = 0; i < nonwritable_page_count; ++i){
        set_virtaddr_flags((void*)(ro + i*4096), PAGE_PRESENT);
    }

    
    //now look for the .init.text
    Elf32_Shdr *init_section = NULL;
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (strcmp(shstrtab + shdr[i].sh_name, ".init.text") == 0) {
            init_section = &shdr[i];
            break;
        }
    }

    if(!init_section){
        r->eax = -ENOEXEC;
        return;
    }
    
    //call these functions
    size_t count = init_section->sh_size / sizeof(void*);
    for (size_t i = 0; i < count; i++) {
        void (**init_fn)(void) = (void (**)(void))init_section->sh_addr;
        
        if (init_fn[i]) init_fn[i]();
    }
    
    r->eax = 0;

    kfree(elf_image);

    return;

}


void module_init(){
    install_syscall_handler(SYSCALL_INIT_MODULE, syscall_init_module);
}


int module_call_all_initializers(){
    
    int module_count = (int)(__inittext_end - __inittext_start);
    fb_console_printf("module_count = %u\n", module_count);
    for(int i = 0; i < module_count; ++i){
        void (**func)(void) = (void (**)(void))__inittext_start;
        fb_console_printf("%u -> %x\n", i, func[i]);
        func[i]();
    }
    
    return module_count;
}

