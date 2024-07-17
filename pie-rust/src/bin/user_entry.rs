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
    let msg1 = "RUST ENTRY TASK!!! Start\r\n";
    let msg2 = "RUST ENTRY TASK!!! End\r\n";

    rust_pie::sys::print_raw::ker_print_raw(msg1);

    let child_args = "PROGRAM\0clock_server\0".as_bytes();
    let child_tid = ker_create(PRIO_CLOCK, child_args).unwrap();
    // println!("child_tid = {child_tid}"); 

    let child_args = "PROGRAM\0rpi\0".as_bytes();
    let child_tid = ker_create(3, child_args).unwrap();
    // println!("child_tid = {child_tid}"); 

    let child_args = "PROGRAM\0encoder_server\0".as_bytes();
    let child_tid = ker_create(2, child_args).unwrap();
    // println!("child_tid = {child_tid}"); 

    let child_args = "PROGRAM\0commander\0".as_bytes();
    let child_tid = ker_create(5, child_args).unwrap();
    // println!("child_tid = {child_tid}"); 

    wait_ticks(300);

    ker_create(3, b"PROGRAM\0imu_server\0").unwrap();

    rust_pie::sys::print_raw::ker_print_raw(msg2);
}
