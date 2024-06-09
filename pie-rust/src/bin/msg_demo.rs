#![no_std]
#![no_main]

use core::panic::PanicInfo;
use msgbox_macro::*;
use rust_pie::sys::msgbox::*;
use rust_pie::sys::syscall::*;
use rust_pie::println;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[repr(C)]
#[derive(Debug, Default)]
#[allow(dead_code)]
struct Embed {
    embed: u64,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
#[allow(dead_code)]
struct MsgDemoReq1<'a> {
    // Please use 'a for the lifetime if needed, MsgTrait relies on it
    simple_field: u64,
    array_field: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
#[allow(dead_code)]
struct MsgDemoReq2<'a> {
    array_field: AttachedArray<'a, Embed>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
#[allow(dead_code)]
struct MsgDemoResp {
    simple_field: u16,
    embed_field: Embed,
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    MsgDemoReq1(&'a MsgDemoReq1<'a>),
    MsgDemoReq2(&'a MsgDemoReq2<'a>),
    MsgDemoResp(&'a MsgDemoResp),
}

#[no_mangle]
pub extern "C" fn _start() {
    let child_args = "PROGRAM\0msg_demo_child\0".as_bytes();
    let child_tid = ker_create(1, child_args).unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::new();

    /* First Communication */

    ker_recv(&mut recv_box);

    let recv_enum = RecvEnum::from_recv_bytes(&mut recv_box);
    println!("recv_enum\r\n{:?}", recv_enum);

    {
        let mut resp = SendCtx::<MsgDemoResp>::new(&mut send_box).unwrap();
        resp.simple_field = 20;
        resp.embed_field.embed = 200;
    }

    ker_reply(child_tid, &send_box).unwrap();

    /* Second Communication */

    ker_recv(&mut recv_box);

    let recv_enum = RecvEnum::from_recv_bytes(&mut recv_box);
    println!("recv_enum\r\n{:?}", recv_enum);

    {
        let mut resp = SendCtx::<MsgDemoResp>::new(&mut send_box).unwrap();
        resp.simple_field = 21;
        resp.embed_field.embed = 201;
    }

    ker_reply(child_tid, &send_box).unwrap();
}
