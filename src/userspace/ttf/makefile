CC=i686-pc-silk-gcc
CFLAGS= -Wall -Wextra -g -I$(HOME)/opt/silk/output/silk_gcc/i686-pc-silk/include/freetype2 
default: main.c
	$(CC) $(CFLAGS) main.c bitmap.c -o ttf -lfreetype -lz
	#cp ttf ../../../build/init_bin/
