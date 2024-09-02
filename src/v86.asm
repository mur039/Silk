[bits 16]
[org 0x7c00]
[section .text]

_start:
    mov ah, 0 ;set video mode
    mov al, 0x0;video mode 80*25 text mode
    ;0xa000

    int 0x10

    ;call memset16

    int 3
    jmp $


;mov ah, 0 ;set video mode
;mov al, 3;video mode 80*25 text mode
;int 0x10
;int 3


memset16:
    ; Save registers that we will use
    push ax
    push bx
    push di
    push es

    ; Set up segment and offset
    mov eax, 0
    mov es, ax  ; Load segment address from the stack (16-bit segment)
    mov di, 0xa000    ; Load offset address from the stack (16-bit offset)
    mov al, 0    ; Load the value to set into AL
    mov cx, 640   ; Load the count of bytes to set into CX

    ; Main loop
fill_loop:
    stosb            ; Store byte at ES:DI and increment DI
    loop fill_loop   ; Repeat until CX is zero

    ; Restore registers
    pop es
    pop di
    pop bx
    pop ax

    ret