include ../common.mk

LDFLAGS:=-Wl,-r

SUBDIRS := test
OBJECTS := $(patsubst %, bin/%, $(SUBDIRS))

all: $(OBJECTS)

clean:
	$(MAKE) -C test clean
	rm -rf bin/

$(OBJECTS): bin/%: 
	$(dir_guard) bin/
	$(MAKE) -C $(patsubst bin/%,%,$@)
	# cp $(patsubst build/%.o,%,$@)/$@ $@

dump:
	@$(OBJDUMP) -d -j .text bin/test

elf:
	@$(READELF) -a bin/test

.PHONY: $(OBJECTS)