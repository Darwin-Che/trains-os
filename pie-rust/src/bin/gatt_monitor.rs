#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::api::name_server::*;
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

const DEBUG: bool = false;

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    GattMonitorReq(&'a mut GattMonitorReq),
    GattMonitorResp(&'a mut GattMonitorResp<'a>),
    GattServerMonitorReq(&'a mut GattServerMonitorReq<'a>),
    GattServerMonitorResp(&'a mut GattServerMonitorResp<'a>),
}

#[no_mangle]
pub extern "C" fn _start(ptr: *const c_char, len: usize) {
    let args = EntryArgs::new(ptr, len);
    args.print();

    let name = args.get("name").unwrap();

    let parent_tid = ker_parent_tid();
    let gatt_server = ns_get("rpi_bluetooth_gatt").unwrap();

    let mut send_box_gatt_server: SendBox = SendBox::default();
    let mut server_monitor_req = SendCtx::<GattServerMonitorReq>::new(&mut send_box_gatt_server).unwrap();
    server_monitor_req.name = server_monitor_req.attach_array(name.len()).unwrap();
    server_monitor_req.name.copy_from_slice(name.as_bytes());

    let mut send_box_parent: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    loop {
        ker_send(gatt_server, &send_box_gatt_server, &mut recv_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::GattServerMonitorResp( GattServerMonitorResp{bytes: bytes} )) => {
                let mut parent_resp = SendCtx::<GattMonitorResp>::new(&mut send_box_parent).unwrap();
                parent_resp.bytes = parent_resp.attach_array(bytes.len()).unwrap();
                parent_resp.bytes.copy_from_slice(&bytes);

                ker_send(parent_tid, &send_box_parent, &mut recv_box).unwrap();
            },
            _ => {
                panic!("[GATT Monitor] Unexpected Response");
            },
        }
    }
}