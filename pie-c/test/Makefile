include ../../common.mk

LDFLAGS:=-Wl,--no-dynamic-linker -Wl,-pie

SOURCES := $(wildcard src/*.c) $(wildcard src/*.S)
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst src/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst src/%.S, build/%.d, $(SOURCES)))

CFLAGS += -I../..

all: ../bin/test.img

clean:
	rm -f $(OBJECTS) $(DEPENDS)
	rm -rf build/

../bin/test.img: ../bin/test.elf
	$(OBJCOPY) $< -O binary $@

../bin/test.elf: $(OBJECTS)
	$(dir_guard) ../bin
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS) -L../../lib/build -lsyscall -lbase
	$(dir_guard) ../dump
	@$(OBJDUMP) -d $@ > ../dump/test.elf

build/%.o: src/%.c Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
	@$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/%.o: src/%.S Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)