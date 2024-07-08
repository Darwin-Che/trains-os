#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::clock::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::log;
use rust_pie::println;

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

#[no_mangle]
pub extern "C" fn _start() {
    log!("Start Encoder VCC at pin 25");
    // start the encoder voltage pin25
    unsafe {
        setup_gpio(25, 0x01, 0x02); // Pullup output
        set_outpin_gpio(25);
    }

    wait_ticks(300);

    log!("ker_quadrature_encoder_init");
    // register the encoder
    let encoder = ker_quadrature_encoder_init(23, 24).unwrap();

    log!("[Encoder] encoder = {}", encoder);
    loop {
        let val = ker_quadrature_encoder_get(encoder);
        log!("[Encoder] ker_quadrature_encoder_get {:?}", val);
        wait_ticks(50); // Every 0.5 sec
    }
}