
global setUserMode:
setUserMode:

    mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax ; SS is handled by iret

	; set up the stack frame iret expects
	mov eax, esp
	push (4 * 8) | 3 ; data selector
	push eax ; current esp
	pushfd ; eflags
	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push 0x200000 ; instruction address to return to
	iret










    mov ebx, 20
    or ebx, 0x00000003
    mov eax, 20
    or eax, 0x00000003

    mov es, eax
    mov gs, eax
    mov fs, eax
    
    push ss
    push esp
    pushfd
    push ebx
    push 0x200000
    iret

        