all: build/user_entry_elf.o build/msg_demo_elf.o build/msg_demo_child_elf.o

TARGET=aarch64-unknown-linux-gnu

build/%_elf.o: bin/%
	mkdir -p build/
	aarch64-none-elf-objcopy -I binary -O elf64-littleaarch64 $(patsubst build/%_elf.o,bin/%,$@) $@

bin/%: cargo
	mkdir -p bin/ dump/
	cp $(patsubst bin/%,target/$(TARGET)/release/%,$@) bin
	aarch64-none-elf-strip -g $@
	aarch64-none-elf-objdump -D $@ > $(patsubst bin/%,dump/%_objdump.txt,$@)
	@aarch64-none-elf-objdump -d -j .text $@ | fgrep -q q0 && printf "\n***** WARNING: SIMD INSTRUCTIONS DETECTED! *****\n\n" || true
	aarch64-none-elf-readelf -a $@ > $(patsubst bin/%,dump/%_readelf.txt,$@)

cargo: src/* Cargo.toml
	cargo build -v --release -Z build-std=core,compiler_builtins

clean:
	cargo clean
	rm -rf bin/ target/ dump/ build/

.PHONY: all clean cargo