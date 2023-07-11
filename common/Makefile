include ../common.mk

LDFLAGS:=-Wl,-r

# Source files and include dirs
SOURCES := $(wildcard *.c) $(wildcard *.S)
# Create .o and .d files for every .cc and .S (hand-written assembly) file
OBJECTS := $(patsubst %.c, build/%.o, $(patsubst %.S, build/%.o, $(SOURCES)))
DEPENDS := $(patsubst %.c, build/%.d, $(patsubst %.S, build/%.d, $(SOURCES)))

all: ../build/common.o

clean:
	rm -f $(OBJECTS) $(DEPENDS) ../build/common.o

../build/common.o: $(OBJECTS)
	$(dir_guard)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/%.o: %.c Makefile
	$(dir_guard)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: %.S Makefile
	$(dir_guard)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)