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
struct Embed {
    embed: u64,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
struct MsgDemoReq1<'a> {
    // Please use 'a for the lifetime if needed, MsgTrait relies on it
    simple_field: u64,
    array_field: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
struct MsgDemoReq2<'a> {
    array_field: AttachedArray<'a, Embed>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
struct MsgDemoResp {
    simple_field: u16,
    embed_field: Embed,
}

#[derive(Debug, RecvEnumTrait)]
enum RecvEnum<'a> {
    MsgDemoReq1(&'a MsgDemoReq1<'a>),
    MsgDemoReq2(&'a MsgDemoReq2<'a>),
    MsgDemoResp(&'a MsgDemoResp),
}

#[no_mangle]
pub extern "C" fn _start() {
    let parent_tid = ker_parent_tid();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::new();

    {
        let array_bytes = "array_field".as_bytes();
        let (mut send_ctr, req1) = SendCtr::<MsgDemoReq1>::new(&mut send_box).unwrap();
        req1.array_field = send_ctr.attach_array(array_bytes.len()).unwrap();
        req1.array_field.copy_from_slice(array_bytes);
        req1.simple_field = 100;
    }

    println!("send_box\r\n{:?}", send_box.as_slice());
    ker_send(parent_tid, &send_box, &mut recv_box).unwrap();
    let recv_enum = RecvEnum::from_recv_bytes(&mut recv_box);
    println!("recv_enum\r\n{:?}", recv_enum);

    {
        let (mut send_ctr, req2) = SendCtr::<MsgDemoReq2>::new(&mut send_box).unwrap();
        req2.array_field = send_ctr.attach_array(2).unwrap();
        req2.array_field[0].embed = 101;
        req2.array_field[1].embed = 102;
    }

    println!("send_box\r\n{:?}", send_box.as_slice());
    ker_send(parent_tid, &send_box, &mut recv_box).unwrap();
    let recv_enum = RecvEnum::from_recv_bytes(&mut recv_box);
    println!("recv_enum\r\n{:?}", recv_enum);
}
