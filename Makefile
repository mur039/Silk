
CFLAGS = -ffreestanding -m32 -g -c  -Wall -Wextra -I./include ./src/
default: src/boot.asm

	nasm -f elf32 -g src/boot.asm -o ./boot.o
	i686-elf-gcc $(CFLAGS)lkmain.c -o lkmain.o
	i686-elf-gcc $(CFLAGS)kmain.c -o kmain.o
	i686-elf-ld -T linker.ld boot.o lkmain.o kmain.o -o kernel.elf

iso:
	cp ./kernel.elf ./iso/boot/
	grub-mkrescue -o bootable.iso iso
	
run: bootable.iso
	qemu-system-i386 -m 64M -hda bootable.iso

kernel: kernel.elf
	qemu-system-i386 -m 64M -kernel kernel.elf
debug_kernel:kernel.elf
	qemu-system-i386 -m 64M -kernel kernel.elf -s -S &
		gdb ./kernel.elf  \
		-ex "set architecture i386:x86-64"\
		-ex "target remote localhost:1234" 
		
debug: kernel.elf
	qemu-system-i386 -m 64M -hda bootable.iso -s -S &
	gdb ./kernel.elf  \
	-ex "target remote localhost:1234" 
