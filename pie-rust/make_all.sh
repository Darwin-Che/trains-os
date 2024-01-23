#!/bin/bash

programs=(user_entry)

mkdir build
mkdir bin

for program in $programs; do
  pushd $program
  make
  cp -f bin/* ../bin
  popd
done

for f in bin/*; do
  aarch64-none-elf-objcopy -I binary -O elf64-littleaarch64 ${f} ${f/bin/build}_elf.o
done
