#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::api::clock::*;
use rust_pie::api::name_server::*;
use rust_pie::api::imu::*;
use rust_pie::api::motor::*;
use rust_pie::log;
use rust_pie::bog;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::api::pid::*;

use core::str;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const DEBUG: bool = false;

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    PidTrigger(&'a mut PidTrigger),
}

#[no_mangle]
pub extern "C" fn _start() {
    let parent_tid = ker_parent_tid();
    
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    loop {
        SendCtx::<PidTrigger>::new(&mut send_box).unwrap();
        ker_send(parent_tid, &send_box, &mut recv_box).unwrap();

        wait_ticks(PID_DT_TICK);
    }
}