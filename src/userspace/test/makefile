CFLAGS = -g -Wall -Wextra 


default: start.asm main.c
	
	i686-pc-silk-gcc $(CFLAGS) main.c -o test
	cp test ../../../build/init_bin/
