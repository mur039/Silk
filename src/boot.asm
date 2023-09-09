ALIGN 4
mboot:
    ; Multiboot macros to make a few lines later more readable
    MULTIBOOT_PAGE_ALIGN	equ 1<<0
    MULTIBOOT_MEMORY_INFO	equ 1<<1
    
    MULTIBOOT_GRAPH_MODE    equ 1 << 2
    MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002

    



    MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_GRAPH_MODE
    MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM
    times 4 dd 0
    dd 0
    dd 640
    dd 480
    dd 32
    
    


extern lkmain
global _start
_start:
    mov esp, bootstrap_stack - 0xC0000000
    push eax
    push ebx
    call lkmain    

global enablePaging:
enablePaging: ;enablePaging(unsigned int*);

; load page directory (eax has the address of the page directory) 
   mov eax, [esp+4]
   mov cr3, eax        

; enable paging 
   mov ebx, cr0        ; read current cr0
   or  ebx, 0x80000000 ; set PG .  set pages as read-only for both userspace and supervisor, replace 0x80000000 above with 0x80010000, which also sets the WP bit.
   mov cr0, ebx        ; update cr0
   ret                 ; now paging is enabled

SECTION .bootstrap
times 8192 dd 0 ; 8kB stack
bootstrap_stack:

global bootstrap_pde
ALIGN 4096
bootstrap_pde:
times 4096 dd 0

global bootstrap_pte1
ALIGN 4096
bootstrap_pte1:
times 4096 dd 0

