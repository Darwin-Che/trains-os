include ../common.mk

LDFLAGS:=-Wl,-r

SOURCES := $(wildcard src/*.c) $(wildcard src/*.S) $(wildcard src/*/*.c)
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst src/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst src/%.S, build/%.d, $(SOURCES)))

all: ../build/loader.o

clean:
	rm -f $(OBJECTS) $(DEPENDS) ../build/loader.o
	rm -rf build/

../build/loader.o: $(OBJECTS)
	$(dir_guard) ../build
	$(CC) $(CFLAGS) $(filter-out build/%.ld, $^) -o $@ $(LDFLAGS)

build/%.o: src/%.c Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
	@$(OBJDUMP) -D $@ > $(call dump_file,$@)

build/%.o: src/%.S Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
