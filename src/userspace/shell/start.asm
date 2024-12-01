global _start
extern main ; takes argument in ecx
extern test_function ;int test_function(char ** argv)
extern exit

_start:
    push ebp
    mov ebp, esp

    push esi
    push edi
    
    call main
    push eax ;return
    call exit




