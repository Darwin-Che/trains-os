#![no_std]
#![no_main]

use core::panic::PanicInfo;

use rust_pie::sys::syscall::*;
use rust_pie::api::name_server::*;
use rust_pie::println;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[no_mangle]
pub extern "C" fn _start() {
    let my_tid = ker_my_tid();
    println!("Name Server Demo : NsSet {:?}", my_tid);
    ns_set("name_server_demo").unwrap();
    let ret = ns_get("name_server_demo");
    println!("Name Server Demo : NsGet {:?}", ret);
}
