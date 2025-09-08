#ifndef __MODULE_H__
#define __MODULE_H__


struct ksym {
    const char *name;
    void       *addr;
};


// Basic representation of a loaded kernel module
struct module {
    char name[64];               // Module name (from ELF or filename)
    void *base;                  // Base address where module was loaded
    unsigned long size;                 // Total allocated size (pages)

    
    unsigned section_count;
    struct ksym *sections;
    
    // Sections
    void *text_base;             // Code section
    unsigned long text_size;
    void *rodata_base;           // Read-only data
    unsigned long rodata_size;
    void *data_base;             // Writable data
    unsigned long data_size;
    void *bss_base;              // Zeroed section
    unsigned long bss_size;

    // Symbol handling
    unsigned long n_exported;
    struct ksym *exported_syms; // Array of exported symbols

    // Relocation
    // Elf32_Rel *relocs;           // (optional) relocations
    // unsigned long n_relocs;

    // Entry/exit
    int (*init)(void);           // module_init()
    void (*exit)(void);          // module_exit()

    // State
    int refcount;                // reference count (modules using this one)
    struct module *next;         // linked list of modules
};



extern struct ksym __ksymtab_start[], __ksymtab_end[];
void *ksym_lookup(const char *name);

#define EXPORT_SYMBOL(sym) \
    static const struct ksym __ksym_##sym \
    __attribute__((section(".ksymtab"))) = { #sym, (void*)(sym) }


void module_init();

extern void* __inittext_start[];
extern void* __inittext_end[];
int module_call_all_initializers();



#define MODULE_INIT(ksym) \
    static void * __module_init  __attribute__((used, section(".init.text"))) = (void*)ksym;

#define MODULE_EXIT(ksym) \
    static void * __module_exit  __attribute__((used, section(".fini.text"))) = &ksym;






#endif