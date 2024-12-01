global _start
extern main ; takes argument in ecx
extern exit

extern intermediate ; int intermediate(int argc, char * argv[])
_start:
    
    mov ebp, 0 ;stack frame linked list
    push ebp
    mov ebp, esp

    

    push esi
    push edi

    call main
    push eax  
    call exit

