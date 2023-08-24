default: start.s main.c
	nasm -f elf32 -g start.s
	i686-elf-gcc main.c -c  -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -o main.o
	i686-elf-ld  -T linker.ld -melf_i386 -o program.elf main.o start.o
	i686-elf-objcopy -O binary program.elf program
                         
