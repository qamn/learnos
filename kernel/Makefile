# -I 输入文件的格式
# -S 移除symbol和relocation信息
# -R 移除指定段
all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system ../kernel.bin

# -b 输入文件的格式
# -z muldefs 可以重复定义,只使用其中一个
# -T 提供链接脚本
system:	head.o main.o printk.o entry.o trap.o memory.o interrupt.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o main.o printk.o trap.o memory.o interrupt.o -T Kernel.lds

# -E 只进行预处理
head.o:	head.S
	gcc -E  head.S > head.s
	as --64 -o head.o head.s

entry.o: interrupt/entry.S
	gcc -E  interrupt/entry.S > entry.s
	as --64 -o entry.o entry.s

# -mcmodel=large 程序可访问任何虚拟地址
# -fno-builtin 不识别内建函数
# -m64 生成64位代码
# -c 进行到汇编,但是不链接
main.o:	main.c
	gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c main.c
	
printk.o: print/printk.c
	gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c print/printk.c

trap.o: interrupt/trap.c
	gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c interrupt/trap.c

memory.o: memory/memory.c
	gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c memory/memory.c

interrupt.o:
	gcc -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c interrupt/interrupt.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin

