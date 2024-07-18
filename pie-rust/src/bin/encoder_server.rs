#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::clock::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::log;
use rust_pie::api::name_server::*;
use rust_pie::api::gatt::*;

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const ENCODER_INTERVAL: u64 = 1;

#[no_mangle]
pub extern "C" fn _start() {
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let gatt_server_tid = ns_get_wait("rpi_bluetooth_gatt");

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
        let val = ker_quadrature_encoder_get(encoder);

        cnt += 1;
        acc += val.forward_cnt as f64 - val.backward_cnt as f64;
        if cnt == 10 {
            // log!("[Encoder] ker_quadrature_encoder_get {} {:?}", acc, val);
            let mut encoder_update = SendCtx::<GattServerPublishReq>::new(&mut send_box).unwrap();
            encoder_update.name = encoder_update.attach_array(b"Encoder".len()).unwrap();
            encoder_update.name.copy_from_slice(b"Encoder");
            encoder_update.bytes = encoder_update.attach_array(24).unwrap();
            encoder_update.bytes[0..8].copy_from_slice(&get_cur_tick().to_le_bytes());
            encoder_update.bytes[8..16].copy_from_slice(&(acc * 10.0 / 64.0 / 78.0).to_le_bytes());
            encoder_update.bytes[16..24].copy_from_slice(&0f64.to_le_bytes());

            ker_send(gatt_server_tid, &send_box, &mut recv_box).unwrap();

            cnt = 0;
            acc = 0.0;
        }

        wait_ticks(ENCODER_INTERVAL); // Every 0.5 sec
    }
}