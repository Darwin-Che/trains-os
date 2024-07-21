#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::clock::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::log;
use rust_pie::api::name_server::*;
use rust_pie::api::gatt::*;
use rust_pie::api::encoder::*;

use heapless::LinearMap;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const ENCODER_INTERVAL: u64 = 500 / TICK_MS; // Every 0.5 sec

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    EncoderResp(&'a mut EncoderResp),
}

#[no_mangle]
pub extern "C" fn _start() {
    let encoder_server_tid = ns_get_wait("encoder_server");
    let gatt_server_tid = ns_get_wait("rpi_bluetooth_gatt");

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    loop {
        SendCtx::<EncoderReq>::new(&mut send_box).unwrap();
        ker_send(encoder_server_tid, &send_box, &mut recv_box).unwrap();

        if let Some(RecvEnum::EncoderResp(resp)) = RecvEnum::from_recv_bytes(&mut recv_box) {
            let mut encoder_update = SendCtx::<GattServerPublishReq>::new(&mut send_box).unwrap();
            encoder_update.name = encoder_update.attach_array(b"Encoder".len()).unwrap();
            encoder_update.name.copy_from_slice(b"Encoder");
            encoder_update.bytes = encoder_update.attach_array(24).unwrap();
            encoder_update.bytes[0..8].copy_from_slice(&get_cur_tick().to_le_bytes());
            encoder_update.bytes[8..16].copy_from_slice(&resp.left.to_le_bytes());
            encoder_update.bytes[16..24].copy_from_slice(&resp.right.to_le_bytes());

            ker_send(gatt_server_tid, &send_box, &mut recv_box).unwrap();
        }

        wait_ticks(ENCODER_INTERVAL);
    }
}