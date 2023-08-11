SOURCE = ./src
default: start.asm main.c
	cd ./src;
	nasm -f elf32 start.asm
	i686-elf-gcc -ffreestanding -m32 -g -c main.c -o main.o
	i686-elf-ld -T linker.ld -o kernel.elf start.o main.o
	rm start.o main.o
