CFLAGS = -g -c -Wall -Wextra  -m32 -fno-builtin -I ../libsilk/include/
LDFLAGS = -T linker.ld -l:libsilk.a -L ../libsilk

default: start.asm main.c
	nasm -f elf32 -g start.asm -o start.o
	i686-elf-gcc $(CFLAGS) main.c -o main.o

	i686-elf-ld -L ./  -o reloc  start.o main.o $(LDFLAGS) -lreloc
	cp reloc ../../../build/init_bin/

sharedlib:

	i686-elf-gcc $(CFLAGS) -fpic reloc_lib.c 
	i686-elf-gcc -shared -o libreloc.so reloc_lib.o



