include ../../common.mk

LDFLAGS:=-Wl,--no-dynamic-linker -Wl,-pie

SOURCES := $(wildcard src/*.c) $(wildcard src/*.S)
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst src/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst src/%.S, build/%.d, $(SOURCES)))

all: ../bin/test

clean:
	rm -f $(OBJECTS) $(DEPENDS) ../bin/test
	rm -rf build/

../bin/test: $(OBJECTS)
	$(dir_guard) ../bin
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)

build/%.o: src/%.c Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
	@$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/%.o: src/%.S Makefile include/context_macro.S
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)