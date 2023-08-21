    extern main
    section .text
        push $22
        call main
        ; main has returned, eax is return value
        jmp  $    ; loop forever
