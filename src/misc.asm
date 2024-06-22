global gdt_flush     ; Allows the C code to link to this
extern gp            ; Says that '_gp' is in another file
gdt_flush:
    lgdt [gp]        ; Load the GDT with our '_gp' which is a special pointer
    mov ax, 0x10      ; 0x10 is the offset in the GDT to our data segment 1M offset 0x10*46kB
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush2   ; 0x08 is the offset to our code segment: Far jump!
flush2:
    ret               ; Returns back to the C code!


; C declaration: void flush_tss(void);
global flush_tss
flush_tss:
	mov ax, (5 * 8) | 0 ; fifth 8-byte selector, symbolically OR-ed with 0 to set the RPL (requested privilege level).
	ltr ax
	ret

global jump_usermode
;will take arguments from registers themselves
jump_usermode:

	mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax ; SS is handled by iret
 
	; set up the stack frame iret expects
	mov eax, esp
	push (4 * 8) | 3 ; data selector
	push edi ; current esp
	pushf ; eflags
	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push esi;test_user_function ; instruction address to return to
	iret




    ;mov ebp, esp ;prologue?

    ;;mov ebx, DWORD [esp + 4]
	;mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	;mov ds, ax
	;mov es, ax 
	;mov fs, ax 
	;mov gs, ax ; SS is handled by iret

	;; set up the stack frame iret expects
	;mov eax, ebx
    ;add eax, 0x200
	;push (4 * 8) | 3 ; data selector
	;push eax ; ;current esp
	;pushf ; eflags
	;push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	;push DWORD [ebp + 4] ;instruction address to return to
	;iret