#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::println;
use rust_pie::sys::syscall::*;

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
    let child_tid = ker_create(0, child_args).unwrap();
    println!("child_tid = {child_tid}"); 

    let child_args = "PROGRAM\0rpi\0".as_bytes();
    let child_tid = ker_create(3, child_args).unwrap();
    println!("child_tid = {child_tid}"); 

    let child_args = "PROGRAM\0encoder_server\0".as_bytes();
    let child_tid = ker_create(2, child_args).unwrap();
    println!("child_tid = {child_tid}"); 

    rust_pie::sys::print_raw::ker_print_raw(msg2);
}
