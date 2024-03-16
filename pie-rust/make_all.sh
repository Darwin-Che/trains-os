#!/bin/bash

mkdir -p build

make

for f in bin/*; do
  aarch64-none-elf-objcopy -I binary -O elf64-littleaarch64 ${f} ${f/bin/build}_elf.o
done
