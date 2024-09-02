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