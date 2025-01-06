#ifndef __ELF_H__
#define __ELF_H__

#include <stdint.h>
#include <sys.h>
#include <uart.h>
#include <str.h>
#include <filesystems/tar.h>
#include <pmm.h>
#include <process.h>



// #define ELF_MAGIC 0x7f454c46 // 0x7f 'E', 'L', 'F'

//e_type
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff


//e_machine
#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7
#define EM_MIPS 8
#define EM_AMD_x86 0x3E



//p_type
#define ELF_PT_NULL 0
#define ELF_PT_LOAD 1
#define ELF_PT_DYNAMIC 2
#define ELF_PT_INTERP 3
#define ELF_PT_NOTE 4
#define ELF_PT_SHLIB 5
#define ELF_PT_PHDR 6
#define ELF_PT_LOOS 0x60000000      
#define ELF_PT_HIOS 0x6FFFFFFF      
#define ELF_PT_LOPROC 0x70000000
#define ELF_PT_HIPROC 0x7fffffff

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4 

#define EI_NIDENT 16
typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t type;         // Segment type
    uint32_t offset;       // Segment file offset
    uint32_t vaddr;        // Segment virtual address
    uint32_t paddr;        // Segment physical address
    uint32_t filesz;       // Segment size in file
    uint32_t memsz;        // Segment size in memory
    uint32_t flags;        // Segment flags
    uint32_t align;        // Segment alignment
} Elf32_Phdr;

void * elf_get_entry_address(file_t *file);
int elf_list_program_headers(file_t * file);
int elf_get_filesz(file_t * file);
void * elf_load(pcb_t * process);


#endif