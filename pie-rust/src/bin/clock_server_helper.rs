#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::clock::*;
use rust_pie::log;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
enum RecvEnum<'a> {
    ClockWaitReq(&'a mut ClockWaitReq),
}

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    let parent = ker_parent_tid();
    let clock_server = ns_get("clock_server").unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    loop {
        SendCtx::<ClockWaitResp>::new(&mut send_box).unwrap();

        ker_send(parent, &send_box, &mut recv_box).unwrap();

        let ticks = match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::ClockWaitReq(wait_req)) => {
                wait_req.ticks
            },
            _ => {
                panic!("[ClockServerHelper] Unexpected Resp 1");
            }
        };

        let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
        wait_req.ticks = ticks;

        ker_send(clock_server, &send_box, &mut recv_box).unwrap();
    }
}