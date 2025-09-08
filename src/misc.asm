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
	mov ax, 0x28 ; (5 * 8) | 0 ; fifth 8-byte selector, symbolically OR-ed with 0 to set the RPL (requested privilege level).
	ltr ax
	ret


global enable_interrupts
global disable_interrupts

enable_interrupts:
	sti 
	ret


disable_interrupts:
	cli
	ret


extern dump_stuck_frame
extern schedule
extern dump_registers

global jump_usermode
jump_usermode:
	;mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	;mov ds, ax
	;mov es, ax 
	;mov fs, ax 
	;mov gs, ax ; SS is handled by iret
 
	; set up the stack frame iret expects

	push (4 * 8) | 3 ; data selector, stack segment also
	push 0 ; user stack
	pushf ; eflags
	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push 0  ; instruction address to return to

	;yes i will set up typical interrupt frame for the schedular and directly jump 
	; to the next ready process instead of intermediate empty code block
	;set up what interrupt handlers like timer which calls the schedular expects

	push 0 ;err_code
	push 0 ; supposed interrupt number

	pusha
    push ds
    push es
    push fs
    push gs

	mov ax, 0x10   ; Load the Kernel Data Segment descriptor!
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov eax, esp   ; Push us the stack
    push eax
    mov eax, schedule
    call eax
    pop eax   ;shoudl have the r?

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number	
	iret



;Push(EAX);
;Push(ECX);
;Push(EDX);
;Push(EBX);
;Push(Temp);
;Push(EBP);
;Push(ESI);
;Push(EDI);
;
global promote_to_kproc
promote_to_kproc:

extern test_user_function

global user_mode_test;
user_mode_test:
	mov ax, (4 * 8) | 3 ; ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax ; SS is handled by iret

	; set up the stack frame iret expects
	mov eax, esp
	push (4 * 8) | 3 ; data selector
	push eax ; current esp
	pushf ; eflags
	push (3 * 8) | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push test_user_function ; instruction address to return to
	iret


global resume_kthread
resume_kthread:
	mov eax, [esp + 4]
	mov esp, eax
	
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8
	; unsigned int eip, cs, eflags, useresp, ss;
	iret


	ret
	

;it is called 
global kernl_schedule_shim
kernl_schedule_shim:
  	
	cli

	pushf
    push 0x08              ; kernel CS
    push kernl_schedule_return ; fake EIP

	push 0 ;int code 
	push 0 ; errcode

    pusha
    push ds
    push es
    push fs
    push gs


    mov eax, esp            ; pointer to top-of-stack pseudo-frame
    push eax
    call schedule

kernl_schedule_return:
	
	sti
	ret



;global kernl_schedule_shim
;kernl_schedule_shim:
;  	cli
;    pushf
;    ; push minimal iret frame for resume: CS:EIP and EFLAGS
;    push 0x08              ; kernel CS
;    push kernel_resume_label ; fake EIP
;	push 0
;	push 0
;
;    pusha
;    push ds
;    push es
;    push fs
;    push gs
;
;
;    mov eax, esp            ; pointer to top-of-stack pseudo-frame
;    push eax
;    call schedule

;extern halt
;global kernel_resume_label
;kernel_resume_label:
;    popf
;    pop gs
;    pop fs
;    pop es
;    pop ds
;    popa
;    sti
;    ret
