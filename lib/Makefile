include ../common.mk

LDFLAGS:=-Wl,-r

LIBDIRS := libbase libfdt libslab librbtree libsyscall
SOURCES := $(foreach dir,$(LIBDIRS),$(wildcard $(dir)/*.c) $(wildcard $(dir)/*.S) $(wildcard $(dir)/*/*.c))
LIBS := $(foreach dir,$(LIBDIRS),build/$(dir).a)
OBJECTS := $(patsubst %.c, build/%.o, $(patsubst %.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst %.c, build/%.d, $(patsubst %.S, build/%.d, $(SOURCES)))

# all: ../build/lib.a
all: $(LIBS)

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(LIBS)
	rm -rf build/
	rm -rf dump/

# ../build/lib.a: $(OBJECTS)
# 	$(dir_guard) ../build
# 	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/%.a: $(OBJECTS) Makefile
	$(dir_guard) build/
	$(AR) rcs $@ build/$(patsubst build/%.a,%,$@)/*.o
	# $(RANDLIB) $@
	@$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/%.o: %.c Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
	@$(OBJDUMP) -d $@ > $(call dump_file,$@)

build/%.o: %.S Makefile
	$(dir_guard) $(dir $@) $(call dump_dir,$@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
