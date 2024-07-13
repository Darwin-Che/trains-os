#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::api::clock::*;
use rust_pie::api::name_server::*;
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
    let name_cmd_ack = b"Cmd Ack";
    let gatt_relay = ker_create(3, b"PROGRAM\0gatt_monitor_relay\0name\0Cmd Input\0").unwrap();
    let gatt_server = ns_get("rpi_bluetooth_gatt").unwrap();
    
    let mut send_box_monitor: SendBox = SendBox::default();
    SendCtx::<GattMonitorReq>::new(&mut send_box_monitor).unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    loop {
        ker_send(gatt_relay, &send_box_monitor, &mut recv_box).unwrap();

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

                let mut cmd_ack = SendCtx::<GattServerPublishReq>::new(&mut send_box).unwrap();
                cmd_ack.name = cmd_ack.attach_array(name_cmd_ack.len()).unwrap();
                cmd_ack.name.copy_from_slice(name_cmd_ack);
                cmd_ack.bytes = cmd_ack.attach_array(16).unwrap();
                cmd_ack.bytes[0..8].copy_from_slice(&id.to_le_bytes());
                cmd_ack.bytes[8..16].copy_from_slice(&get_cur_tick().to_le_bytes());

                ker_send(gatt_server, &send_box, &mut recv_box).unwrap();
            },
            _ => {
                panic!("[CMD] Unexpected RecvEnum");
            }
        }
    }
}