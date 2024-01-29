SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

ARCH=cortex-a72
TRIPLE=aarch64-none-elf
CC:=$(TRIPLE)-gcc
OBJSIZE:=$(TRIPLE)-size
OBJCOPY:=$(TRIPLE)-objcopy
OBJDUMP:=$(TRIPLE)-objdump
READELF:=$(TRIPLE)-readelf
STRIP:=$(TRIPLE)-strip
AR:=$(TRIPLE)-ar
RANLIB:=$(TRIPLE)-ranlib

# COMPILE OPTIONS
WARNINGS=-Wall -Wextra -Wpedantic -Wno-unused-const-variable
CFLAGS:=-pipe -static $(WARNINGS) -ffreestanding -nostartfiles\
	-mcpu=$(ARCH) -static-pie -mstrict-align -fno-builtin -mgeneral-regs-only -O3 -Iinclude -I..

dir_guard=@mkdir -p

define dump_dir
	$(patsubst build/%,dump/%,$(dir $1))
endef

define dump_file
	$(subst build/,dump/,$1)
	# $(patsubst build/%.o,dump/%.dp,$1)
endef
