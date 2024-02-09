all: bin/user_entry

bin/user_entry: src/* Cargo.toml rpi4b.json
	cargo build --release -Z build-std=core,compiler_builtins --target rpi4b.json
	mkdir -p bin/ dump/
	cp target/rpi4b/release/user_entry bin/user_entry
	aarch64-none-elf-strip -g bin/user_entry
	aarch64-none-elf-objdump -D bin/user_entry > dump/user_entry_objdump.txt
	aarch64-none-elf-readelf -a bin/user_entry > dump/user_entry_readelf.txt

clean:
	rm -rf bin/ target/

.PHONY: all clean