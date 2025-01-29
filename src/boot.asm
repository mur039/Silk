ALIGN 4
mboot:
    ; Multiboot macros to make a few lines later more readable
    MULTIBOOT_PAGE_ALIGN	equ 1<<0
    MULTIBOOT_MEMORY_INFO	equ 1<<1
    MULTIBOOT_VIDEO_MODE    equ 1<<2
    MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
    MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE
    MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
    
    ;Multiboot Header
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM
    dd 0 ;HeaderAddr
    dd 0 ;LoadAddr
    dd 0 ;Load_endAddr
    dd 0 ;bssEndAddr
    dd 0 ;entryAddr
    dd 0;mode type, linear mode
    dd 1280;width
    dd 800 ;height
    dd 4;depth  

    ;1280x800
    ;


extern lkmain
global _start
_start:
    mov esp, bootstrap_stack - 0xC0000000 ;new stack
    lgdt [GDT_descriptor] ;loading new gdt
    mov ebp, 0 ;for stack trace purposes
    push eax
    push ebx
    call lkmain
    hlt

;-----------------------------------
CODE_SEG equ GDT_code - GDT_start;
DATA_SEG equ GDT_data - GDT_start;

GDT_start:
    GDT_null:
        dd 0x0
        dd 0x0

    GDT_code:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10011011 
        db 0xF
        db 0x0

    GDT_data:
        dw 0xffff
        dw 0x0
        db 0x0
        db 0b10010011
        db 0x0F
        db 0x0
GDT_end:

GDT_descriptor:
    dw GDT_end - GDT_start - 1
    dd GDT_start

;-----------------------------------



global enablePaging:
enablePaging: ;enablePaging(unsigned int*);
   mov eax, [esp+4]
   mov cr3, eax        
; enable paging 
   mov ebx, cr0        ; read current cr0
   or  ebx, 0x80000000 ; set PG .  set pages as read-only for both userspace and supervisor, replace 0x80000000 above with 0x80010000, which also sets the WP bit.
   mov cr0, ebx        ; update cr0
   ret                 ; now paging is enabled


;extern void _changeSP(uint32_t * nsp, int (* foo)(multiboot_info_t * mbd), multiboot_info_t mbd);
global _changeSP
_changeSP:

    mov eax, [esp + 0] ;ret addr
    mov ebx, [esp + 4] ;new stack
    mov ecx, [esp + 8] ;next func
    mov edx, [esp + 12];argument to the next func

   mov esp, ebx
   mov ebp, esp
;   push ebx
;   push eax

    push edx
    call ecx
    

global ie_func:
ie_func:
	jmp $
	

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

global bootstrap_pte2
ALIGN 4096
bootstrap_pte2:
times 4096 dd 0



