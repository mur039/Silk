CFLAGS = -g -c -Wall -Wextra  -m32
default: start.asm main.c
	nasm -f elf32 -g start.asm -o start.o
	i686-elf-gcc $(CFLAGS) main.c -o main.o
	i686-elf-ld -T linker.ld start.o main.o -o userprogram.elf
	cp userprogram.elf ../../build/
