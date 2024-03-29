MAKEFLAGS += -I ..

include common.mk

LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld

all: kernel8.img

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C user clean
	$(MAKE) -C lib clean
	$(MAKE) -C pie-rust clean
	rm -f kernel8.img kernel8.elf
	rm -rf build/
	rm -rf dump/

size:
	$(OBJSIZE) kernel8.elf

kernel8.img: kernel8.elf
	$(OBJCOPY) $< -O binary $@

kernel8.elf: linker.ld lib build/kernel.o build/loader.o pie-rust
	$(dir_guard) build/
	$(CC) $(CFLAGS) build/kernel.o build/loader.o pie-rust/build/* -o $@ $(LDFLAGS) -Llib/build -lbase -lrbtree -lslab -lsyscall
	@$(OBJDUMP) -d -j .text kernel8.elf | fgrep -q q0 && printf "\n***** WARNING: SIMD INSTRUCTIONS DETECTED! *****\n\n" || true
	$(OBJDUMP) -D $@ > dump/kernel8.objdump
	$(READELF) -a $@ > dump/kernel8.readelf

.PHONY: lib build/kernel.o build/loader.o pie-rust

lib:
	$(MAKE) -C lib/

build/kernel.o: lib
	$(dir_guard) build/
	$(dir_guard) dump/
	$(MAKE) -C kernel
	$(STRIP) -g $@
	$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/loader.o: lib
	$(dir_guard) build/
	$(dir_guard) dump/
	$(MAKE) -C loader
	$(STRIP) -g $@
	$(OBJDUMP) -d $@ > $(call dump_file,$@)

pie-rust: lib
	$(MAKE) -C pie-rust
	# cd pie-rust && bash make_all.sh
