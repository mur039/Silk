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


/* sh_type */
 #define SHT_NULL    0
 #define SHT_PROGBITS    1
 #define SHT_SYMTAB  2
 #define SHT_STRTAB  3
 #define SHT_RELA    4
 #define SHT_HASH    5
 #define SHT_DYNAMIC 6
 #define SHT_NOTE    7
 #define SHT_NOBITS  8
 #define SHT_REL     9
 #define SHT_SHLIB   10
 #define SHT_DYNSYM  11
 #define SHT_NUM     12
 #define SHT_LOPROC  0x70000000
 #define SHT_HIPROC  0x7fffffff
 #define SHT_LOUSER  0x80000000
 #define SHT_HIUSER  0xffffffff
 
 /* sh_flags */
 #define SHF_WRITE   0x1
 #define SHF_ALLOC   0x2
 #define SHF_EXECINSTR   0x4
 #define SHF_MASKPROC    0xf0000000
 
 /* special section indexes */
 #define SHN_UNDEF   0
 #define SHN_LORESERVE   0xff00
 #define SHN_LOPROC  0xff00
 #define SHN_HIPROC  0xff1f
 #define SHN_ABS     0xfff1
 #define SHN_COMMON  0xfff2
 #define SHN_HIRESERVE   0xffff


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

 typedef struct elf32_shdr {
    uint32_t    sh_name;
    uint32_t    sh_type;
    uint32_t    sh_flags;
    uint32_t    sh_addr;
    uint32_t sh_offset;
    uint32_t    sh_size;
    uint32_t    sh_link;
    uint32_t    sh_info;
    uint32_t    sh_addralign;
    uint32_t    sh_entsize;
  } Elf32_Shdr;


 typedef struct {
    uint32_t r_offset;
    uint32_t   r_info;
} Elf32_Rel;

typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
    uint32_t r_addend; // explicit addend
} Elf32_Rela;


#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))

#define R_386_NONE	0	//No relocation
#define R_386_32	1	//Direct 32-bit: S + A
#define R_386_PC32	2	//PC-relative 32-bit: S + A - P
#define R_386_GOT32	3	//32-bit Global Offset Table entry (GOT)
#define R_386_PLT32	4	//32-bit Procedure Linkage Table entry (for function calls)
#define R_386_COPY	5	//Copy symbol at runtime (used by dynamic linker)
#define R_386_GLOB_DAT	6	//Set GOT entry to the address of the symbol
#define R_386_JMP_SLOT	7	//Set PLT entry for a function
#define R_386_RELATIVE	8	//Add base address to symbol-less relocation
#define R_386_GOTOFF	9	//32-bit offset relative to GOT
#define R_386_BASE32	10	//32-bit absolute address of shared object

void * elf_get_entry_address(file_t *file);
int elf_list_program_headers(file_t * file);
int elf_get_filesz(file_t * file);
void * elf_load(pcb_t * process);


typedef struct elf32_sym{
  uint32_t    st_name;
  uint32_t    st_value;
  uint32_t    st_size;
  unsigned char st_info;
  unsigned char st_other;
  uint16_t    st_shndx;
} Elf32_Sym;


#endif