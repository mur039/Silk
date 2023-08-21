SOURCE = ./src
CFLAGS = -ffreestanding -m32 -g -c  -O1 -Wall -Wextra
OBJECTS = start.o gdt.o system.o main.o vga.o idt.o isrs.o irq.o timer.o kb.o uart.o k_heap.o page_enable.o paging.o
default: start.asm main.c
	nasm -f elf32 -g start.asm
	nasm -f elf32 -g page_enable.asm
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
	i686-elf-gcc -I./include $(CFLAGS) k_heap.c -o k_heap.o
	i686-elf-gcc -I./include $(CFLAGS) paging.c -o paging.o
	objcopy -I binary -O elf32-i386 usr/program ./usrProgram.o
	i686-elf-ld -T linker.ld -o kernel.elf $(OBJECTS) usrProgram.o
	rm $(OBJECTS)

	
	
debug: kernel.elf gdbcommands
	qemu-system-x86_64 -m 64M -kernel kernel.elf -s -S &
	gdb ./kernel.elf -x gdbcommands
iso: kernel.elf
	grub-mkrescue -o os.iso isodir
	qemu-system-x86_64 -m 64M -hda os.iso 
	
kernel: kernel.elf
	qemu-system-x86_64 -m 64M -kernel kernel.elf
