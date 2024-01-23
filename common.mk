SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

include $(SELF_DIR)/../xdir.mk

ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CC:=$(XBINDIR)/$(TRIPLE)-gcc
OBJSIZE:=$(XBINDIR)/$(TRIPLE)-size
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump
READELF:=$(XBINDIR)/$(TRIPLE)-readelf
STRIP:=$(XBINDIR)/$(TRIPLE)-strip
AR:=$(XBINDIR)/$(TRIPLE)-ar
RANLIB:=$(XBINDIR)/$(TRIPLE)-ranlib

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
