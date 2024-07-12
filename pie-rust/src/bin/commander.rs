#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::log;
use rust_pie::sys::syscall::*;

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
    GattMonitorResp(&'a mut GattMonitorResp<'a>),
}

#[no_mangle]
pub extern "C" fn _start() {
    let gatt_relay = ker_create(3, b"PROGRAM\0gatt_monitor_relay\0name\0Cmd Input\0").unwrap();
    
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    SendCtx::<GattMonitorReq>::new(&mut send_box).unwrap();

    loop {
        ker_send(gatt_relay, &send_box, &mut recv_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::GattMonitorResp(monitor_resp)) => {
                let id = if monitor_resp.bytes.len() >= 8 {
                    u64::from_le_bytes(monitor_resp.bytes[0..8].try_into().unwrap())
                } else {
                    0
                };

                let cmd = if monitor_resp.bytes.len() >= 8 {
                    str::from_utf8(&monitor_resp.bytes[8..]).unwrap()
                } else {
                    str::from_utf8(&monitor_resp.bytes[..]).unwrap()
                }; 

                log!("[CMD] {} {}", id, cmd);

                
            },
            _ => {
                panic!("[CMD] Unexpected RecvEnum");
            }
        }
    }
}