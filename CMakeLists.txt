cmake_minimum_required(VERSION 3.17)
project(learnos)

set(CMAKE_CXX_STANDARD 14)

include_directories(kernel)
include_directories(kernel/interrupt)
include_directories(kernel/lib)
include_directories(kernel/print)

add_executable(learnos
        bootloader/boot.asm
        bootloader/loader.asm
        bootloader/Makefile
        kernel/interrupt/trap.c
        kernel/interrupt/trap.h
        kernel/lib/lib.h
        kernel/lib/linkage.h
        kernel/print/font.h
        kernel/print/printk.c
        kernel/print/printk.h
        kernel/head.S
        kernel/Kernel.lds
        kernel/main.c
        kernel/Makefile
        addfile.sh
        bochsrc
        cmd.txt
        README.md kernel/interrupt/gate.h kernel/memory/memory.c kernel/memory/memory.h kernel/interrupt/interrupt.c kernel/interrupt/interrupt.h)
