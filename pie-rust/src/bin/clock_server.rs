#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::log;
use rust_pie::println;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const QUANTUM: u64 = 10 * 1000; // us

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    ns_set("clock_server").unwrap();

    let mut clock = RpiClock::new();
    let mut t = clock.cur_u64();
    clock.intr_clear();

    if DEBUG {
        println!("[Clock] Start {}", t);
    }

    loop {
        // Set the interrupt
        t += QUANTUM;
        clock.intr_arm(t);
        if DEBUG {
            println!("[Clock] Set Interrupt {}", clock.cur_u64());
        }

        // Wait for the interrupt
        ker_await_clock().unwrap();
        if DEBUG {
            println!("[Clock] Wake from Interrupt");
        }
    }
}