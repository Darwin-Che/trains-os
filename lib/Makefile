include ../common.mk

LDFLAGS:=-Wl,-r

LIBDIRS := base libfdt
SOURCES := $(foreach dir,$(LIBDIRS),$(wildcard $(dir)/*.c) $(wildcard $(dir)/*.S) $(wildcard $(dir)/*/*.c))
OBJECTS := $(patsubst %.c, build/%.o, $(patsubst %.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst %.c, build/%.d, $(patsubst %.S, build/%.d, $(SOURCES)))

all: ../build/lib.o

clean:
	rm -f $(OBJECTS) $(DEPENDS) ../build/lib.o
	rm -rf build/
	rm -rf dump/

../build/lib.o: $(OBJECTS)
	$(dir_guard) ../build
	$(CC) $(CFLAGS) $(filter-out build/%.ld, $^) -o $@ $(LDFLAGS)

build/%.o: %.c Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
	@$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/%.o: %.S Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
