    extern main
    section .text
        call main
        ; main has returned, eax is return value
        jmp  $    ; loop forever
