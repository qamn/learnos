#! /bin/bash

sudo mount ./boot.img /media/floppy0 -t vfat -o loop
sudo cp ./$1 /media/floppy0/
sync
sudo umount /media/floppy0