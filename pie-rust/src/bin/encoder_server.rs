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

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const ENCODER_INTERVAL: u64 = 1;

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    EncoderReq(&'a mut EncoderReq),
    EncoderResp(&'a mut EncoderResp),
}

#[no_mangle]
pub extern "C" fn _start() {
    ns_set("encoder_server").unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    ker_create(PRIO_ENCODER, b"PROGRAM\0encoder_collector\0").unwrap();

    let mut registry : LinearMap<Tid, EncoderResp, 8> = LinearMap::new();

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::EncoderReq(_)) => {
                let mut reply = SendCtx::<EncoderResp>::new(&mut send_box).unwrap();
                if let Some(v) = registry.get_mut(&sender_tid) {
                    **reply = *v;
                    v.left = 0.0;
                    v.right = 0.0;
                } else {
                    registry.insert(sender_tid, EncoderResp{ left: 0.0, right: 0.0 }).unwrap();
                }
                ker_reply(sender_tid, &send_box).unwrap();
            },
            Some(RecvEnum::EncoderResp(resp)) => {
                SendCtx::<EncoderReq>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap();
                for (_, val) in registry.iter_mut() {
                    val.add(resp);
                }
            },
            _ => {
                log!("[ENCODER] Unexpected Receive");
            }
        }
    }
}