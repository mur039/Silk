[bits 16]
[org 0x7c00]
[section .text]


struc VesaInfoBlock				;	VesaInfoBlock_size = 512 bytes
	.Signature		resb 4		;	must be 'VESA'
	.Version		resw 1
	.OEMNamePtr		resd 1
	.Capabilities		resd 1

	.VideoModesOffset	resw 1
	.VideoModesSegment	resw 1

	.CountOf64KBlocks	resw 1
	.OEMSoftwareRevision	resw 1
	.OEMVendorNamePtr	resd 1
	.OEMProductNamePtr	resd 1
	.OEMProductRevisionPtr	resd 1
	.Reserved		resb 222
	.OEMData		resb 256
endstruc




_start:
    push ds
    pop es
    mov di, VesaInfoBlockBuffer

    mov ax, 0x4f00
    int 0x10
    cmp ax, 0x004f
    
    je .vbe_success
    mov al, 1
    mov di, _failed_vbe_str
    int 3

.vbe_success:
    mov al, 1
    mov di, _success_vbe_str
    int 3
    
    
    mov di, VesaInfoBlockBuffer
    mov al, 2
    mov bx, 4
    int 3

    mov al, 3
    mov bx, 512
    int 3

    mov cl, [VesaInfoBlockBuffer + VesaInfoBlock.VideoModesSegment]
    mov es, cx 
    mov di, [VesaInfoBlockBuffer + VesaInfoBlock.VideoModesOffset]
    mov al, 3
    mov bx, 2*4
    int 3


	mov bx, [VesaInfoBlockBuffer + VesaInfoBlock.VideoModesOffset]	;	get video modes list address

;    push word [VesaInfoBlockBuffer + VesaInfoBlock.VideoModesSegment]
;	pop es
;	mov di, VesaModeInfoBlockBuffer

	mov cx, [bx]							;	get first video mode number
	cmp cx, 0xffff							;	vesa modes list empty




    jmp exit
    jmp $


exit:
    mov al, 0
    int 3

_failed_vbe_str:
    db "Failed to execute VBE command", 0  

_success_vbe_str:
    db "Succesfully execute VBE command", 0  

_str:
    db "Hello", 0  



ALIGN(4)

	VesaInfoBlockBuffer: istruc VesaInfoBlock
		at VesaInfoBlock.Signature,				db "VESA"
		times 508 db 0
	iend