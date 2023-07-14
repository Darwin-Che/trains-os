include ../common.mk

LDFLAGS:=-Wl,-r

SOURCES := $(wildcard src/*.c) $(wildcard src/*.S)
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst src/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst src/%.S, build/%.d, $(SOURCES)))

all: ../build/kernel.o

clean:
	rm -f $(OBJECTS) $(DEPENDS) ../build/kernel.o
	rm -rf build/

../build/kernel.o: $(OBJECTS)
	$(dir_guard) ../build
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)

build/%.o: src/%.c Makefile
	$(dir_guard)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: src/%.S Makefile include/context_macro.S
	$(dir_guard)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
