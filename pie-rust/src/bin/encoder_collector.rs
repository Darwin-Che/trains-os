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

    log!("Start Encoder VCC at pin 12 and 17");
    // start the encoder voltage pin12
    unsafe {
        setup_gpio(12, GPIO_SETTING_OUTPUT, GPIO_RESISTOR_PDP); // Pullup output
        setup_gpio(17, GPIO_SETTING_OUTPUT, GPIO_RESISTOR_PDP); // Pullup output
        set_outpin_gpio(12);
        set_outpin_gpio(17);
    }

    wait_ms(1000);

    log!("ker_quadrature_encoder_init");
    // register the encoder
    let encoder_left = ker_quadrature_encoder_init(16, 20).unwrap();
    let encoder_right = ker_quadrature_encoder_init(27, 22).unwrap();

    log!("[Encoder] encoder = ({}, {})", encoder_left, encoder_right);

    loop {
        wait_ticks(1);

        let val_left = ker_quadrature_encoder_get(encoder_left);
        let val_right = ker_quadrature_encoder_get(encoder_right);

        let mut encoder_update = SendCtx::<EncoderResp>::new(&mut send_box).unwrap();
        encoder_update.left = (val_left.forward_cnt as f64 - val_left.backward_cnt as f64) / ENCODER_RATIO;
        encoder_update.right = (val_right.forward_cnt as f64 - val_right.backward_cnt as f64) / ENCODER_RATIO;

        ker_send(parent_tid, &send_box, &mut recv_box).unwrap();
    }
}