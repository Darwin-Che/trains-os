MAKEFLAGS += -I ..

include common.mk

LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld

SUBDIRS := kernel user lib
OBJECTS := $(patsubst %, build/%.o, $(SUBDIRS))

all: kernel8.img

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C user clean
	$(MAKE) -C lib clean
	rm -f $(OBJECTS) kernel8.img kernel8.elf
	rm -rf build/
	rm -rf dump/

size:
	$(OBJSIZE) kernel8.elf

kernel8.img: kernel8.elf
	$(OBJCOPY) $< -O binary $@

kernel8.elf: $(OBJECTS) linker.ld
	$(dir_guard) build/
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)
	@$(OBJDUMP) -d -j .text kernel8.elf | fgrep -q q0 && printf "\n***** WARNING: SIMD INSTRUCTIONS DETECTED! *****\n\n" || true

.PHONY: $(OBJECTS)

$(OBJECTS): build/%.o: 
	$(dir_guard) build/
	$(dir_guard) dump/
	$(MAKE) -C $(patsubst build/%.o,%,$@)
	$(OBJDUMP) -d $@ > $(call dump_file,$@)
	# cp $(patsubst build/%.o,%,$@)/$@ $@

-include $(DEPENDS)
