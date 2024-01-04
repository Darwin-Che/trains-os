include ../common.mk

LDFLAGS:=-Wl,-r

SUBDIRS := test
OBJECTS := $(patsubst %, bin/%, $(SUBDIRS))

all: $(OBJECTS) build/test.o

clean:
	$(MAKE) -C test clean
	rm -rf bin/
	rm -rf build/
	rm -rf dump/

$(OBJECTS): bin/%: 
	$(dir_guard) bin/
	$(MAKE) -C $(patsubst bin/%,%,$@)
	# cp $(patsubst build/%.o,%,$@)/$@ $@

build/test.o:
	$(dir_guard) build
	$(OBJCOPY) -I binary -O elf64-littleaarch64 bin/test.img build/test.o
	$(READELF) -a $@ > dump/test.o

dump:
	@$(OBJDUMP) -d -j .text bin/test

elf:
	@$(READELF) -a bin/test

.PHONY: $(OBJECTS)