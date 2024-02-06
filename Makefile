
CFLAGS = -ffreestanding -m32 -g -c  -Wall -Wextra -I./include ./src/
default: src/boot.asm

	nasm -f elf32 -g src/boot.asm -o ./boot.o
	nasm -f elf32 -g src/irq.asm -o ./irq_asm.o
	nasm -f elf32 -g src/isr.asm -o ./isr_asm.o
	i686-elf-gcc $(CFLAGS)lkmain.c -o lkmain.o
	i686-elf-gcc $(CFLAGS)kmain.c -o kmain.o
	i686-elf-gcc $(CFLAGS)uart.c -o uart.o
	i686-elf-gcc $(CFLAGS)acpi.c -o acpi.o
	i686-elf-gcc $(CFLAGS)string.c -o string.o
	i686-elf-gcc $(CFLAGS)idt.c -o idt.o
	i686-elf-gcc $(CFLAGS)isr.c -o isr.o
	i686-elf-gcc $(CFLAGS)irq.c -o irq.o
	i686-elf-ld -T linker.ld boot.o lkmain.o kmain.o string.o uart.o acpi.o idt.o isr.o isr_asm.o irq.o irq_asm.o -o kernel.elf 

iso:
	cp ./kernel.elf ./iso/boot/
	grub-mkrescue -o bootable.iso iso
	
run: bootable.iso
	qemu-system-i386 -m 512M -hda bootable.iso

kernel: kernel.elf
	qemu-system-i386 -m 512M -kernel kernel.elf
debug_kernel:kernel.elf
	qemu-system-i386 -m 512M -kernel kernel.elf -s -S &
		gdb ./kernel.elf  \
		-ex "set architecture i386:x86-64"\
		-ex "target remote localhost:1234" 
		
debug: kernel.elf
	qemu-system-i386 -m 512M -hda bootable.iso -s -S &
	gdb ./kernel.elf  \
	-ex "target remote localhost:1234" 
