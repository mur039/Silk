global _start
extern main ; takes argument in ecx
extern exit

_start:
    mov ebp, 0

    push ebp
    mov ebp, esp

    push esi ;char **
    push edi ;int 

    call main
    push eax ;return
    call exit

_wait:
    hlt
    jmp _wait