CC=i686-pc-silk-gcc
CFLAGS = -g -Wall -Wextra
LDFLAGS = -T linker.ld -l:libsilk.a -L ../libsilk

default: start.asm main.c
	
	$(CC) $(CFLAGS) main.c glyph.c -o gsh
	cp gsh ../../../build/init_bin/
