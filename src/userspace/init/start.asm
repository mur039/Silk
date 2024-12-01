global _start
extern main ; 
extern exit
_start:
    
    mov ebp, 0 ;stack frame linked list
    push ebp
    mov ebp, esp

    push esi
    push edi

    call main
    push eax  
    call exit
