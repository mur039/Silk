SOURCE = ./src
CFLAGS = -ffreestanding -m32 -g -c  -Wall -Wextra
default: start.asm main.c
	nasm -f elf32 -g start.asm
	i686-elf-gcc -I./include $(CFLAGS) main.c -o main.o
	i686-elf-gcc -I./include $(CFLAGS) vga.c -o vga.o
	i686-elf-gcc -I./include $(CFLAGS) system.c -o system.o
	i686-elf-gcc -I./include $(CFLAGS) gdt.c -o gdt.o
	i686-elf-gcc -I./include $(CFLAGS) idt.c -o idt.o
	i686-elf-gcc -I./include $(CFLAGS) isrs.c -o isrs.o
	i686-elf-gcc -I./include $(CFLAGS) irq.c -o irq.o
	i686-elf-gcc -I./include $(CFLAGS) timer.c -o timer.o
	i686-elf-gcc -I./include $(CFLAGS) kb.c -o kb.o
	i686-elf-gcc -I./include $(CFLAGS) uart.c -o uart.o
	i686-elf-gcc -I./include $(CFLAGS) alloc.c -o alloc.o

	i686-elf-ld -T linker.ld -o kernel.elf start.o gdt.o system.o main.o vga.o idt.o isrs.o irq.o timer.o kb.o uart.o alloc.o
	rm start.o main.o vga.o system.o gdt.o idt.o isrs.o irq.o timer.o kb.o uart.o alloc.o
kernel: kernel.elf
	qemu-system-x86_64 -m 64M -kernel kernel.elf -s &
