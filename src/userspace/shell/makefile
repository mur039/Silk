CFLAGS = -g -c -Wall -Wextra  -m32 -fno-builtin -I ../libsilk/include/ -I ./
LDFLAGS = -T linker.ld -l:libsilk.a -L ../libsilk

default: start.asm main.c
	nasm -f elf32 -g start.asm -o start.o
	i686-elf-gcc $(CFLAGS) main.c -o main.o
	i686-elf-gcc $(CFLAGS) token.c -o token.o
	i686-elf-gcc $(CFLAGS) arena.c -o arena.o
	i686-elf-ld -o sh start.o main.o token.o arena.o $(LDFLAGS)
	cp sh ../../../build/init_bin/
