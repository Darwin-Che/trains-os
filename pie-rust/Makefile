all: build/user_entry_elf.o build/msg_demo_elf.o

build/%_elf.o: bin/%
	mkdir -p build/
	aarch64-none-elf-objcopy -I binary -O elf64-littleaarch64 $(patsubst build/%_elf.o,bin/%,$@) $@

bin/%: cargo
	mkdir -p bin/ dump/
	cp $(patsubst bin/%,target/rpi4b/release/%,$@) bin
	aarch64-none-elf-strip -g $@
	aarch64-none-elf-objdump -D $@ > $(patsubst bin/%,dump/%_objdump.txt,$@)
	aarch64-none-elf-readelf -a $@ > $(patsubst bin/%,dump/%_readelf.txt,$@)

cargo: src/* Cargo.toml rpi4b.json
	cargo build -v -v --release -Z build-std=core,compiler_builtins --target rpi4b.json

clean:
	rm -rf bin/ target/ dump/ build/

.PHONY: all clean cargo