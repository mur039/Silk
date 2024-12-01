
global _syscall

_syscall:
    push ebp
    mov ebp, esp
    
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    
    

    mov eax, [ebp + 8] ;syscall number 
    mov ebx, [ebp + 12] ; arg0 
    mov ecx, [ebp + 16] ; arg1 
    mov edx, [ebp + 20] ; arg2
    mov esi, [ebp + 24] ; arg3 
    mov edi, [ebp + 28] ; arg4 
    mov ebp, [ebp + 32] ; arg5 

    int 0x80

    pop ebp

    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx

    pop ebp
    ret