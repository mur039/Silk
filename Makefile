BUILD_DIR = ./build/
CFLAGS = -ffreestanding -m32 -g -c  -Wall -Wextra -I./include ./src/
default: src/boot.asm

	nasm -f elf32 -g src/boot.asm -o $(BUILD_DIR)boot.o
	nasm -f elf32 -g src/irq.asm -o  $(BUILD_DIR)irq_asm.o
	nasm -f elf32 -g src/isr.asm -o  $(BUILD_DIR)isr_asm.o
	nasm -f elf32 -g src/misc.asm -o $(BUILD_DIR)misc_asm.o

	
	make userspace
	i686-elf-gcc $(CFLAGS)lkmain.c -o $(BUILD_DIR)lkmain.o
	i686-elf-gcc $(CFLAGS)kmain.c -o $(BUILD_DIR)kmain.o
	i686-elf-gcc $(CFLAGS)uart.c -o $(BUILD_DIR)uart.o
	i686-elf-gcc $(CFLAGS)acpi.c -o $(BUILD_DIR)acpi.o
	i686-elf-gcc $(CFLAGS)str.c -o $(BUILD_DIR)str.o
	i686-elf-gcc $(CFLAGS)gdt.c -o $(BUILD_DIR)gdt.o
	i686-elf-gcc $(CFLAGS)idt.c -o $(BUILD_DIR)idt.o
	i686-elf-gcc $(CFLAGS)isr.c -o $(BUILD_DIR)isr.o
	i686-elf-gcc $(CFLAGS)irq.c -o $(BUILD_DIR)irq.o
	i686-elf-gcc $(CFLAGS)pmm.c -o $(BUILD_DIR)pmm.o
	i686-elf-gcc $(CFLAGS)pit.c -o $(BUILD_DIR)pit.o
	i686-elf-gcc $(CFLAGS)kb.c -o  $(BUILD_DIR)kb.o
	i686-elf-gcc $(CFLAGS)timer.c -o $(BUILD_DIR)timer.o
	
	i686-elf-gcc $(CFLAGS)filesystems/tar.c -o $(BUILD_DIR)tar.o
	i686-elf-gcc $(CFLAGS)filesystems/vfs.c -o $(BUILD_DIR)vfs.o
	
	i686-elf-gcc $(CFLAGS)glyph.c -o $(BUILD_DIR)glyph.o
	i686-elf-gcc $(CFLAGS)fb.c -o $(BUILD_DIR)fb.o
	i686-elf-gcc $(CFLAGS)syscalls.c -o $(BUILD_DIR)syscalls.o
	i686-elf-gcc $(CFLAGS)pci.c -o $(BUILD_DIR)pci.o
	i686-elf-gcc $(CFLAGS)process.c -o $(BUILD_DIR)process.o
	i686-elf-gcc $(CFLAGS)elf.c -o $(BUILD_DIR)elf.o
	i686-elf-gcc $(CFLAGS)g_list.c -o $(BUILD_DIR)g_list.o
	cd $(BUILD_DIR) && i686-elf-ld -o ../kernel.elf -T ../linker.ld \
		boot.o lkmain.o \
		kmain.o str.o uart.o acpi.o idt.o isr.o isr_asm.o irq.o irq_asm.o \
		pmm.o pit.o gdt.o misc_asm.o kb.o timer.o tar.o glyph.o fb.o syscalls.o \
		pci.o elf.o process.o vfs.o g_list.o

SUBDIRS := $(wildcard src/userspace/*/) 
COMMAND := make default
userspace:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir ; \
		done
init: ./initrd
	cp build/init_bin/* ./initrd/tar_files/bin/
	cd initrd && make
	
iso:
	make init
	cp ./kernel.elf ./iso/boot/
	cp ./initrd/init.tar ./iso/modules/init.tar
	grub-mkrescue -o bootable.iso iso
	
run: bootable.iso
	make
	make iso -B
	qemu-system-i386 -m 512M -hda bootable.iso

kernel: kernel.elf
	qemu-system-i386 -m 512M -kernel kernel.elf -initrd iso/modules/userspaceprogram.bin -initrd iso/modules/init.tar
debug_kernel:kernel.elf
	qemu-system-i386 -m 512M -kernel kernel.elf -s -S &
		gdb ./kernel.elf  \
		-ex "set architecture i386:x86-64"\
		-ex "target remote localhost:1234" 
		
debug: kernel.elf
	qemu-system-i386 -m 512M -hda bootable.iso -s -S 
