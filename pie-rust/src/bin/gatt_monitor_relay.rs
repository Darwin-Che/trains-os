#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::log;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;

use heapless::String;
use heapless::Deque;
use heapless::Vec;

use core::fmt::Write;

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
}

#[no_mangle]
pub extern "C" fn _start(ptr: *const c_char, len: usize) {
    let args = EntryArgs::new(ptr, len);
    args.print();

    let name = args.get("name").unwrap();

    let mut create_args: String<256> = String::new();
    write!(&mut create_args, "PROGRAM\0gatt_monitor\0name\0{}\0", name);

    let monitor = ker_create(3, create_args.as_bytes()).unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut send_box_monitor: SendBox = SendBox::default();
    SendCtx::<GattMonitorReq>::new(&mut send_box_monitor).unwrap();

    // Can buffer 64 Updates in total
    // Each Message can be at most 512 bytes long
    let mut queue = Deque::<Vec::<u8, 512>, 64>::new();

    let mut watcher: Option<Tid> = None;

    loop {
        let sender = ker_recv(&mut recv_box);
        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::GattMonitorResp(monitor_resp)) => {
                ker_reply(sender, &send_box_monitor).unwrap();
                if let Some(w) = watcher {
                    let mut relay_resp = SendCtx::<GattMonitorResp>::new(&mut send_box).unwrap();
                    relay_resp.bytes = relay_resp.attach_array(monitor_resp.bytes.len()).unwrap();
                    relay_resp.bytes.copy_from_slice(&monitor_resp.bytes);
                    ker_reply(w, &send_box).unwrap();
                    watcher = None;
                } else {
                    let v = Vec::from_slice(&monitor_resp.bytes).unwrap();
                    queue.push_back(v).unwrap();
                }
            },
            Some(RecvEnum::GattMonitorReq(_)) => {
                if watcher.is_some() {
                    panic!("[GATT Monitor Relay] Watcher is already set");
                }

                if let Some(v) = queue.pop_front() {
                    let mut relay_resp = SendCtx::<GattMonitorResp>::new(&mut send_box).unwrap();
                    relay_resp.bytes = relay_resp.attach_array(v.len()).unwrap();
                    relay_resp.bytes.copy_from_slice(&v);
                    ker_reply(sender, &send_box).unwrap(); 
                } else {
                    watcher = Some(sender);
                }
            },
            _ => {
                panic!("[GATT Monitor Relay] Unexpected RecvEnum");
            },
        }
    }
}