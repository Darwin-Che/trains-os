MAKEFLAGS += -I ..

include common.mk

LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld

all: kernel8.img

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C user clean
	$(MAKE) -C lib clean
	$(MAKE) -C pie-c clean
	rm -f kernel8.img kernel8.elf
	rm -rf build/
	rm -rf dump/

size:
	$(OBJSIZE) kernel8.elf

kernel8.img: kernel8.elf
	$(OBJCOPY) $< -O binary $@

kernel8.elf: linker.ld lib build/kernel.o build/user.o pie-c
	$(dir_guard) build/
	$(CC) $(CFLAGS) build/kernel.o build/user.o pie-c/build/test.o -o $@ $(LDFLAGS) -Llib/build -lbase -lrbtree -lslab -lsyscall
	@$(OBJDUMP) -d -j .text kernel8.elf | fgrep -q q0 && printf "\n***** WARNING: SIMD INSTRUCTIONS DETECTED! *****\n\n" || true
	$(OBJDUMP) -d -j .pie $@ > dump/kernel8.elf

.PHONY: lib build/kernel.o build/user.o pie-c

lib:
	$(MAKE) -C lib/

build/kernel.o: lib
	$(dir_guard) build/
	$(dir_guard) dump/
	$(MAKE) -C kernel
	$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/user.o: lib
	$(dir_guard) build/
	$(dir_guard) dump/
	$(MAKE) -C user
	$(OBJDUMP) -d $@ > $(call dump_file,$@)

pie-c: lib
	$(MAKE) -C pie-c

# -include $(DEPENDS)
