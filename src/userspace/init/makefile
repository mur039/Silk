CFLAGS = -g -c -Wall -Wextra  -m32 -fno-builtin -I ../libsilk/include/
LDFLAGS = -T linker.ld -l:libsilk.a -L ../libsilk


default: main.c
	i686-pc-silk-gcc -g -Wall -Wextra main.c -o init
	cp init ../../../build/init_bin/

oldsilk: start.asm main.c
	nasm -f elf32 -g start.asm -o start.o
	i686-elf-gcc $(CFLAGS) main.c -o main.o
	i686-elf-ld -o init  start.o main.o $(LDFLAGS)
	cp init ../../../build/init_bin/

