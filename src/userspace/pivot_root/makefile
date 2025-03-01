CFLAGS = -g -c -Wall -Wextra  -m32 -fno-builtin -I ../libsilk/include/
LDFLAGS = -T linker.ld -l:libsilk.a -L ../libsilk

default: start.asm main.c
	nasm -f elf32 -g start.asm -o start.o
	i686-elf-gcc $(CFLAGS) main.c -o main.o
	i686-elf-ld -o mount start.o main.o $(LDFLAGS)
	cp mount ../../../build/init_bin/
