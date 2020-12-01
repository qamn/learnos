all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kernel.bin

system:	head.o main.o 
	ld -b elf64-x86-64 -o system head.o main.o -T Kernel.lds 

head.o:	head.S
	gcc -E  head.S > head.s
	as --64 -o head.o head.s
	
main.o:	main.c
	gcc  -mcmodel=large -fno-builtin -m64 -c main.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system