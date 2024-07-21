#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::clock::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::log;
use rust_pie::api::encoder::*;

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const ENCODER_RATIO: f64 = 64.0 * 78.0;


#[no_mangle]
pub extern "C" fn _start() {
    let parent_tid = ker_parent_tid();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

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

    let mut cnt = 0;
    let mut acc: f64 = 0.0;

    loop {
        wait_ticks(1);

        let val = ker_quadrature_encoder_get(encoder);

        let mut encoder_update = SendCtx::<EncoderResp>::new(&mut send_box).unwrap();
        encoder_update.left = (val.forward_cnt as f64 - val.backward_cnt as f64) / ENCODER_RATIO;
        encoder_update.right = 0.0;

        ker_send(parent_tid, &send_box, &mut recv_box).unwrap();
    }
}