include ../common.mk

LDFLAGS:=-Wl,-r

SOURCES := $(wildcard src/*.c) $(wildcard src/*.S) $(wildcard src/*/*.c)
OBJECTS := $(patsubst src/%.c, build/%.o, $(patsubst src/%.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst src/%.c, build/%.d, $(patsubst src/%.S, build/%.d, $(SOURCES)))

all: ../build/user.o

clean:
	rm -f $(OBJECTS) $(DEPENDS) ../build/user.o
	rm -rf build/

../build/user.o: $(OBJECTS)
	$(dir_guard) ../build
	$(CC) $(CFLAGS) $(filter-out build/%.ld, $^) -o $@ $(LDFLAGS)

build/%.o: src/%.c Makefile
	$(dir_guard) $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: src/%.S Makefile
	$(dir_guard) $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)