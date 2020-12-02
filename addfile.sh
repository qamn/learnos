#! /bin/bash

sudo mount ./a.img /media/floppy0 -t vfat -o loop
sudo cp ./$1 /media/floppy0/
sync
sudo umount /media/floppy0