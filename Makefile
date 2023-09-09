
CFLAGS = -ffreestanding -m32 -g -c  -Wall -Wextra -I./include ./src/
default: src/boot.asm
	nasm -f elf32 -g src/boot.asm -o ./boot.o
	i686-elf-gcc $(CFLAGS)lkmain.c -o lkmain.o
	i686-elf-gcc $(CFLAGS)kmain.c -o kmain.o
	i686-elf-ld -T linker.ld boot.o  lkmain.o kmain.o -o kernel.elf