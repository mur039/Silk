BUILD_DIR = ./build
CFLAGS = -ffreestanding -m32 -g -c -Wextra -I./include -DDEBUG
default: 
	make kernel
	make libsilk
	make userspace
	make iso -B

SUBDIRS := $(wildcard src/userspace/*/) 
COMMAND := make default
userspace:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir ; \
		done
init: ./initrd
	cp build/init_bin/* ./initrd/tar_files/bin/
	cp $(BUILD_DIR)/v86.bin ./initrd/tar_files/bin/v86.bin
	cd initrd && make
	

libsilk:
	make -C ./src/userspace/libsilk

iso:
	make init
	cp ./kernel.elf ./iso/boot/
	cp ./initrd/init.tar ./iso/modules/init.tar
	grub-mkrescue -o bootable.iso iso
	
run: bootable.iso
	qemu-system-i386 -m 512M -hda bootable.iso

run_virtio: bootable.iso
	qemu-system-i386 -m 512M -hda bootable.iso -vga virtio

run_harddisk: bootable.iso blk_file.iso
	qemu-system-i386 -m 512M -drive file=bootable.iso,media=cdrom,index=1 -drive file=blk_file.iso,media=disk,index=0,format=raw -boot d

kernel: 

	nasm -f elf32 -g src/boot.asm -o $(BUILD_DIR)/boot.o
	nasm -f elf32 -g src/irq.asm -o  $(BUILD_DIR)/irq_asm.o
	nasm -f elf32 -g src/isr.asm -o  $(BUILD_DIR)/isr_asm.o
	nasm -f elf32 -g src/misc.asm -o $(BUILD_DIR)/misc_asm.o

	nasm -f bin src/v86.asm -o $(BUILD_DIR)/v86.bin
	

	


	i686-elf-gcc $(CFLAGS) ./src/lkmain.c -o $(BUILD_DIR)/lkmain.o
	i686-elf-gcc $(CFLAGS) ./src/kmain.c -o $(BUILD_DIR)/kmain.o
	i686-elf-gcc $(CFLAGS) ./src/uart.c -o $(BUILD_DIR)/uart.o
	i686-elf-gcc $(CFLAGS) ./src/acpi.c -o $(BUILD_DIR)/acpi.o
	i686-elf-gcc $(CFLAGS) ./src/str.c -o $(BUILD_DIR)/str.o
	i686-elf-gcc $(CFLAGS) ./src/gdt.c -o $(BUILD_DIR)/gdt.o
	i686-elf-gcc $(CFLAGS) ./src/idt.c -o $(BUILD_DIR)/idt.o
	i686-elf-gcc $(CFLAGS) ./src/isr.c -o $(BUILD_DIR)/isr.o
	i686-elf-gcc $(CFLAGS) ./src/irq.c -o $(BUILD_DIR)/irq.o
	i686-elf-gcc $(CFLAGS) ./src/pmm.c -o $(BUILD_DIR)/pmm.o
	i686-elf-gcc $(CFLAGS) ./src/pit.c -o $(BUILD_DIR)/pit.o
	i686-elf-gcc $(CFLAGS) ./src/tty.c -o $(BUILD_DIR)/tty.o

	i686-elf-gcc $(CFLAGS) ./src/v86.c -o $(BUILD_DIR)/v86.o
	i686-elf-gcc $(CFLAGS) ./src/semaphore.c -o $(BUILD_DIR)/semaphore.o
	i686-elf-gcc $(CFLAGS) ./src/queue.c -o $(BUILD_DIR)/queue.o

	i686-elf-gcc $(CFLAGS) ./src/dev.c -o  $(BUILD_DIR)/dev.o
	i686-elf-gcc $(CFLAGS) ./src/char.c -o $(BUILD_DIR)/char.o
	i686-elf-gcc $(CFLAGS) ./src/kb.c -o  $(BUILD_DIR)/kb.o
	i686-elf-gcc $(CFLAGS) ./src/ps2.c -o  $(BUILD_DIR)/ps2.o
	i686-elf-gcc $(CFLAGS) ./src/ps2_mouse.c -o  $(BUILD_DIR)/ps2_mouse.o
	i686-elf-gcc $(CFLAGS) ./src/cmos.c -o $(BUILD_DIR)/cmos.o
	i686-elf-gcc $(CFLAGS) ./src/timer.c -o $(BUILD_DIR)/timer.o
	
	i686-elf-gcc $(CFLAGS) ./src/filesystems/tar.c -o $(BUILD_DIR)/tar.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/vfs.c -o $(BUILD_DIR)/vfs.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/ata.c -o $(BUILD_DIR)/ata.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/nulldev.c -o $(BUILD_DIR)/nulldev.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/proc.c -o $(BUILD_DIR)/proc.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/fat.c -o $(BUILD_DIR)/fat.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/ext2.c -o $(BUILD_DIR)/ext2.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/tmpfs.c -o $(BUILD_DIR)/tmpfs.o
	i686-elf-gcc $(CFLAGS) ./src/filesystems/pex.c -o $(BUILD_DIR)/pex.o
	i686-elf-gcc $(CFLAGS) ./src/virtio.c -o $(BUILD_DIR)/virtio.o

	i686-elf-gcc $(CFLAGS) ./src/glyph.c -o $(BUILD_DIR)/glyph.o
	i686-elf-gcc $(CFLAGS) ./src/fb.c -o $(BUILD_DIR)/fb.o
		i686-elf-gcc $(CFLAGS) ./src/bosch_vga.c -o $(BUILD_DIR)/bosch_vga.o
	i686-elf-gcc $(CFLAGS) ./src/syscalls.c -o $(BUILD_DIR)/syscalls.o
	i686-elf-gcc $(CFLAGS) ./src/pci.c -o $(BUILD_DIR)/pci.o
	i686-elf-gcc $(CFLAGS) ./src/process.c -o $(BUILD_DIR)/process.o
	i686-elf-gcc $(CFLAGS) ./src/elf.c -o $(BUILD_DIR)/elf.o
	i686-elf-gcc $(CFLAGS) ./src/g_list.c -o $(BUILD_DIR)/g_list.o
	i686-elf-gcc $(CFLAGS) ./src/g_tree.c -o $(BUILD_DIR)/g_tree.o
	i686-elf-gcc $(CFLAGS) ./src/circular_buffer.c -o $(BUILD_DIR)/circular_buffer.o
	i686-elf-gcc $(CFLAGS) ./src/pipe.c -o $(BUILD_DIR)/pipe.o
	i686-elf-gcc $(CFLAGS) ./src/vmm.c -o $(BUILD_DIR)/vmm.o

	cd $(BUILD_DIR) && i686-elf-ld -o ../kernel.elf -T ../linker.ld  \
		boot.o lkmain.o \
		kmain.o str.o uart.o acpi.o idt.o isr.o isr_asm.o irq.o irq_asm.o \
		pmm.o pit.o gdt.o misc_asm.o ps2.o kb.o timer.o tar.o glyph.o fb.o syscalls.o \
		v86.o pci.o elf.o process.o vfs.o g_list.o circular_buffer.o ps2_mouse.o dev.o \
		ata.o cmos.o virtio.o pipe.o vmm.o queue.o semaphore.o g_tree.o nulldev.o proc.o \
		fat.o ext2.o tmpfs.o tty.o char.o pex.o bosch_vga.o

	
debug_kernel:kernel.elf
	qemu-system-i386 -m 3G -kernel kernel.elf -s -S &
		gdb ./kernel.elf  \
		-ex "set architecture i386:x86-64"\
		-ex "target remote localhost:1234" 
		
debug: kernel.elf
	qemu-system-i386 -m 3G -hda bootable.iso -s -S 
