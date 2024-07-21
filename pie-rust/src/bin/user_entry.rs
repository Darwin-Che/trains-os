#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::println;
use rust_pie::sys::syscall::*;
use rust_pie::sys::types::*;
use rust_pie::api::clock::*;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[no_mangle]
pub extern "C" fn _start() {
    ker_create(PRIO_CLOCK, b"PROGRAM\0clock_server\0").unwrap();

    ker_create(3, b"PROGRAM\0rpi\0").unwrap();

    ker_create(PRIO_ENCODER, b"PROGRAM\0encoder_server\0").unwrap();

    ker_create(PRIO_UI, b"PROGRAM\0encoder_reporter\0").unwrap();

    ker_create(PRIO_UI, b"PROGRAM\0commander\0").unwrap();

    wait_ticks(300);

    ker_create(3, b"PROGRAM\0imu_server\0").unwrap();

    ker_create(3, b"PROGRAM\0motor_server\0").unwrap();

    ker_create(PRIO_PID, b"PROGRAM\0pid\0").unwrap();
}
