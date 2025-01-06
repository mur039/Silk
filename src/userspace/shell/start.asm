global _start
extern main ; takes argument in ecx
extern malloc_init ;int test_function(char ** argv)
extern exit

_start:
    push ebp
    mov ebp, esp

    push esi
    push edi

    call malloc_init

    pop edi
    pop esi
    
    call main
    push eax ;return
    call exit




