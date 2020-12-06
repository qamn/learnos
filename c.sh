#!/bin/bash

cd kernel
make clean
make
make clean
cd ..
./addfile.sh kernel.bin