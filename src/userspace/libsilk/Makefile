NASMFLAGS = -g -f elf32
CFLAGS = -g -c -Wall -Wextra  -shared -m32 -fno-builtin -I./include
default:
	$(foreach file, $(wildcard ./*.c), \
	i686-elf-gcc $(CFLAGS) $(file) -o $(file).o; )

	$(foreach file, $(wildcard ./*.asm), \
	nasm $(NASMFLAGS) $(file) -o $(file).o; )

	i686-elf-ar -rcs libsilk.a  $(foreach file, $(wildcard ./*.o), $(file))

	
 
