#!/bin/bash

programs=(user_entry)

rm -rf build
rm -rf bin

for program in $programs; do
  pushd $program
  make clean
  popd
done

