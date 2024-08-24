global _start
extern main ; takes argument in ecx

_start:
    push ebp
    mov ebp, esp
    push ebx ;char **
    push eax ;int 

    call main
    push eax ;return
    call exit


global read  ;int read(int fd);
global write ; size_t write(int fd, const void *buf, size_t count);
global open ; open(char * s, int mode)
global lseek ; lseek(int fd, off_t offset, int whence)
global close ; close(int fd)
global exit ; exit(int code)

write:
    push ebp
    mov ebp, esp

    mov eax, 1 ;write
    mov ebx, [ebp + 8] ; fd
    mov ecx, [ebp + 12] ;buf 
    mov edx, [ebp + 16] ; count

    int 0x80
    pop ebp
    ret

lseek:
    push ebp
    mov ebp, esp

    mov eax, 4 ;lseek
    mov ebx, [ebp + 8] ; fd
    mov ecx, [ebp + 12] ;offset 
    mov edx, [ebp + 16] ; whence

    int 0x80
    pop ebp
    ret

open:
    push ebp
    mov ebp, esp
    mov eax, 2
    mov ebx, [ebp + 8]
    mov ecx, [ebp + 12]
    int 0x80
    pop ebp
    ret

read:
    push ebp
    mov ebp, esp

    mov eax, 0
    mov ebx, [ebp + 8]
    int 0x80
    pop ebp
    ret


close:
    push ebp
    mov ebp, esp

    mov eax, 3
    mov ebx, [ebp + 8] ;fd
    int 0x80
    pop ebp
    ret


exit:
    push ebp
    mov ebp, esp

    mov eax, 60
    mov ebx, [ebp + 8] ;fd
    int 0x80
    pop ebp
    ret
    
global execve
execve:
    push ebp
    mov ebp, esp

    mov eax, 59
    mov ebx, [ebp + 8] ;pathname
    mov ecx, [ebp + 12] ;args

    int 0x80
    pop ebp
    ret



global fstat
fstat:
    push ebp
    mov ebp, esp

    mov eax, 5
    mov ebx, [ebp + 8] ;fd
    mov ecx, [ebp + 12] ;stat_t *

    int 0x80
    pop ebp
    ret