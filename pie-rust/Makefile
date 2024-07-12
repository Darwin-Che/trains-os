PROGRAMS=
PROGRAMS+=user_entry
PROGRAMS+=msg_demo
PROGRAMS+=msg_demo_child
PROGRAMS+=name_server
PROGRAMS+=name_server_demo
PROGRAMS+=rpi
PROGRAMS+=rpi_uart
PROGRAMS+=rpi_uart_intr_broker
PROGRAMS+=rpi_bluetooth_commander
PROGRAMS+=rpi_bluetooth_hci_rx
PROGRAMS+=rpi_bluetooth_gatt
PROGRAMS+=clock_server
PROGRAMS+=clock_server_helper
PROGRAMS+=clock_notifier
PROGRAMS+=encoder_server
PROGRAMS+=gatt_monitor
PROGRAMS+=gatt_monitor_relay
PROGRAMS+=commander

PROGRAMS_ELF=$(patsubst %,build/%_elf.o,$(PROGRAMS))

all: $(PROGRAMS_ELF)

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