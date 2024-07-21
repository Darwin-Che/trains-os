ENTRY(_start)           /* ELF entry symbol - no strictly needed */

SECTIONS {
  /* Page Tabl */
  /* Arbitrary, aligned with 4KB (0x1000) */
  /* 2^18 = 256KB */
  . = 0x40000;
  PROVIDE(LINKER_CODE_START = .);
  id_pgd = .;
	.pd_pgd : { . += (6 * (1 << 12)); }

  /* Kernel Segment */
  /* 2^19 = 512KB */
  . = 0x80000;
	.text : {
    KEEP("build/kernel.o"(.text.boot))
    build/kernel.o (.text*)
    build/loader.o (.text*)
	}

  /* 2 * 2^20 = 2MB */
  . = 0x00200000;

  .rodata : {
  }

  .data : {
    build/kernel.o (.data*)
    build/loader.o (.data*)
  }

  .eh_frame : {
  }

  . = ALIGN(16);
  .bss : {

  }


  /* 4 * 2^20 = 4MB */
  . = 0x00400000;
  PROVIDE(KERNEL_STACK_START = .);
  /* 8 * 2^20 = 8MB */
  . = 0x00800000;
  PROVIDE(KERNEL_STACK_END = .);

  /* (3*16+12) * 2^20 = 60MB */
  /* . = 0x03c00000; */

  /* (4*16) * 2^20 64MB */
  . = 0x4000000;
  PROVIDE(user_entry_elf_start = .);
  .user_entry_elf : {
    pie-rust/build/user_entry_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(msg_demo_elf_start = .);
  .msg_demo_elf : {
    pie-rust/build/msg_demo_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(msg_demo_child_elf_start = .);
  .msg_demo_child_elf : {
    pie-rust/build/msg_demo_child_elf.o (.*)
  }
  
  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(name_server_elf_start = .);
  .name_server_elf : {
    pie-rust/build/name_server_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(name_server_demo_elf_start = .);
  .name_server_demo_elf : {
    pie-rust/build/name_server_demo_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(rpi_elf_start = .);
  .rpi_elf : {
    pie-rust/build/rpi_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(rpi_uart_elf_start = .);
  .rpi_uart_elf : {
    pie-rust/build/rpi_uart_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(rpi_uart_intr_broker_elf_start = .);
  .rpi_uart_intr_broker_elf : {
    pie-rust/build/rpi_uart_intr_broker_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(rpi_bluetooth_commander_elf_start = .);
  .rpi_bluetooth_commander_elf : {
    pie-rust/build/rpi_bluetooth_commander_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(rpi_bluetooth_hci_rx_elf_start = .);
  .rpi_bluetooth_hci_rx_elf : {
    pie-rust/build/rpi_bluetooth_hci_rx_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(rpi_bluetooth_gatt_elf_start = .);
  .rpi_bluetooth_gatt_elf : {
    pie-rust/build/rpi_bluetooth_gatt_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(clock_server_elf_start = .);
  .clock_server_elf : {
    pie-rust/build/clock_server_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(clock_server_helper_elf_start = .);
  .clock_server_helper_elf : {
    pie-rust/build/clock_server_helper_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(clock_notifier_elf_start = .);
  .clock_notifier_elf : {
    pie-rust/build/clock_notifier_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(encoder_server_elf_start = .);
  .encoder_server_elf : {
    pie-rust/build/encoder_server_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(encoder_reporter_elf_start = .);
  .encoder_reporter_elf : {
    pie-rust/build/encoder_reporter_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(encoder_collector_elf_start = .);
  .encoder_collector_elf : {
    pie-rust/build/encoder_collector_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(gatt_monitor_elf_start = .);
  .gatt_monitor_elf : {
    pie-rust/build/gatt_monitor_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(gatt_monitor_relay_elf_start = .);
  .gatt_monitor_relay_elf : {
    pie-rust/build/gatt_monitor_relay_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(commander_elf_start = .);
  .commander_elf : {
    pie-rust/build/commander_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(imu_server_elf_start = .);
  .imu_server_elf : {
    pie-rust/build/imu_server_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(imu_collector_elf_start = .);
  .imu_collector_elf : {
    pie-rust/build/imu_collector_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(motor_server_elf_start = .);
  .motor_server_elf : {
    pie-rust/build/motor_server_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(pid_elf_start = .);
  .pid_elf : {
    pie-rust/build/pid_elf.o (.*)
  }

  . = ALIGN(0x10000); /* 2^16 = 64KB */
  PROVIDE(pid_trigger_elf_start = .);
  .pid_trigger_elf : {
    pie-rust/build/pid_trigger_elf.o (.*)
  }

  PROVIDE(LINKER_CODE_END = .);
}
