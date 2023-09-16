[bits 16]
global getVbeInfo
global getVbeModeInfo
global _16start
extern lkmain 
extern k16main

_16start:
    mov eax, cr0
    xor eax, 1 
    mov cr0, eax
    jmp 0:_16real

 _16real:
    xor eax, eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    lidt[idt_real] ;setting interrupt descriptor to bios interrupts
    sti
    ; yey now i have bios bitches
;--------------------------------
    mov sp, _realmode_stack

    call k16main


APM_shutdown:
    mov ah,53h    
    mov al,00h    
    xor bx,bx     
    int 15h       
    jc APM_error    
    
    mov ah,53h               
    mov al, 01h
    xor bx,bx                
    int 15h                  
    jc APM_error             

    mov ah,53h               ;this is an APM command
    mov al,0eh               ;set driver supported version command
    mov bx,0000h             ;device ID of system BIOS
    mov ch,01h               ;APM driver major version
    mov cl,01h               ;APM driver minor version (can be 01h or 02h if the latter one is supported)
    int 15h
    jc APM_error

    mov ah,53h              ;this is an APM command
    mov al,08h              ;Change the state of power management...
    mov bx,0001h            ;...on all devices to...
    mov cx,0001h            ;...power management on.
    int 15h                 ;call the BIOS function through interrupt 15h
    jc APM_error 

    ;Set the power state for all devices
    mov ah,53h              ;this is an APM command
    mov al,07h              ;Set the power state...
    mov bx,0001h            ;...on all devices to...
    mov cx,0x3              ;see above
    int 15h                 ;call the BIOS function through interrupt 15h
    jc APM_error            ;if the carry 

APM_error:
    mov ah, 0xe
    mov al, '!'
    int 0x10
    jmp $


getVbeInfo:  ;int getVBEInfo(struct VbeInfoBlock * vib)
    push ebp ;gcc -m16 makes function calls and argument passing with 32 bit opcodes
    mov ebp, esp

    mov eax, [ebp + 8]
    mov di, ax

    xor ax, ax
    mov es, ax

    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F

    mov ax, 1 ; yaas
    je .ok0
;başarısız
    mov ax, 0
.ok0:
    pop ebp
    ret

getVbeModeInfo: ;int getVbeModeInfo(struct VBEModeInfoBlock  *vmib, uint16_t mode)
    push ebp ;gcc -m16 makes function calls and argument passing with 32 bit opcodes
    mov ebp, esp

    mov eax, [ebp + 8]
    mov ecx, [ebp + 12] ; mode?

    mov di, ax
    xor ax, ax
    mov es, ax

    mov ax, 0x4F01
    int 0x10
    cmp ax, 0x004F

    mov ax, 1 ; yaas
    je .ok1
;başarısız
    mov ax, 0
.ok1:
    pop ebp
    ret

global setVbeMode
setVbeMode: ;int setVbeMode(uint16_t mode)
    push ebp ;gcc -m16 makes function calls and argument passing with 32 bit opcodes
    mov ebp, esp

    mov ebx, [ebp + 8]
    or ebx, 0x4000
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F

    mov ax, 1 ; yaas
    je .ok2
;başarısız
    mov ax, 0
.ok2:
    pop ebp
    ret



global queryModes
queryModes:
    push ebp 
    mov ebp, esp

    mov eax, [ebp + 8] ; seg
    mov ebx, [ebp + 12] ; offset

    mov fs, ax
    mov si, bx
    
    mov eax, fs:[si]  ; ret
    
    pop ebp
    ret



SECTION .data
idt_real:
    dw 0x3FF
    dd 0


SECTION .bss
global _realmode_stack
    resb 2048
_realmode_stack:


    

