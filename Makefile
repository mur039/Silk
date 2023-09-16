
CFLAGS = -ffreestanding -m32 -g -c  -Wall -Wextra -I./include ./src/
default: src/boot.asm

	make realmode
	nasm -f elf32 -g src/boot.asm -o ./boot.o
	i686-elf-gcc $(CFLAGS)lkmain.c -o lkmain.o
	i686-elf-gcc $(CFLAGS)kmain.c -o kmain.o
	i686-elf-ld -R rmode.sym --just-symbols= rmode.sym -T linker.ld boot.o lkmain.o kmain.o -o kernel.elf


iso:
	cp ./kernel.elf ./iso/boot/
	grub-mkrescue -o bootable.iso iso
	
run: bootable.iso
	qemu-system-x86_64 -m 64M -hda bootable.iso

kernel: kernel.elf
	qemu-system-x86_64 -m 64M -kernel kernel.elf
debug_kernel:kernel.elf
	qemu-system-x86_64 -m 64M -kernel kernel.elf -s -S &
		gdb ./kernel.elf  \
		-ex "set architecture i386:x86-64"\
		-ex "target remote localhost:1234" 
		
debug: kernel.elf
	qemu-system-x86_64 -m 64M -hda bootable.iso -s -S &
	gdb ./kernel.elf  \
	-ex "set architecture i386:x86-64"\
	-ex "target remote localhost:1234" 
realmode:
	nasm -f elf -g src/16bit/16entry.asm -o ./16entry.o
	i686-elf-gcc -ffreestanding -m16 -g -c  -Wall -Wextra -I./include/16bit ./src/16bit/16kmain.c -o ./16kmain.o 
	i686-elf-gcc -ffreestanding -m16 -g -c  -Wall -Wextra -I./include/16bit ./src/16bit/16tty.c -o ./16tty.o 

	i686-elf-ld -T src/16bit/linked.ld 16entry.o 16kmain.o 16tty.o -o rmode.elf
	i686-elf-objcopy rmode.elf --extract-symbol rmode.sym
	i686-elf-objcopy -O binary rmode.elf rmode.bin
