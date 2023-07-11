#!/bin/bash

case $1 in

  "pi")
    if [[ $# -eq 2 && $2 -ge 0 && $2 -lt 5 ]]; then
      echo "Setting up pi #$2"
      read
      /u/cs452/public/tools/setupTFTP.sh CS01754$2 trainsos.img
      echo "+++++++ Finished +++++++"
      exit
    else
      echo "Unable to complete: $2 is not a valid pi ID"
      exit
    fi
    ;;

  "dump")
    /u/cs452/public/xdev/bin/aarch64-none-elf-objdump -d trainsos.elf | less;;

esac
