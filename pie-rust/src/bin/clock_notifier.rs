#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::api::clock::*;
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

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let clock_server = ker_parent_tid();

    SendCtx::<ClockNotifier>::new(&mut send_box).unwrap();

    loop {
        ker_send(clock_server, &send_box, &mut recv_box).unwrap();
        
        // Wait for the interrupt
        ker_await_clock().unwrap();
        if DEBUG {
            println!("[Clock] Wake from Interrupt");
        }
    }
}