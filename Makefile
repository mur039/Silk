SOURCE = ./src
default: start.asm main.c
	nasm -f elf32 -g start.asm
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c main.c -o main.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c vga.c -o vga.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c system.c -o system.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c gdt.c -o gdt.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c idt.c -o idt.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c isrs.c -o isrs.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c irq.c -o irq.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c timer.c -o timer.o
	i686-elf-gcc -I./include -ffreestanding -m32 -g -c kb.c -o kb.o

	i686-elf-ld -T linker.ld -o kernel.elf start.o gdt.o system.o main.o vga.o idt.o isrs.o irq.o timer.o kb.o
	rm start.o main.o vga.o system.o gdt.o idt.o isrs.o irq.o timer.o kb.o
kernel: kernel.elf
	qemu-system-x86_64 -m 64M -kernel kernel.elf -s &
