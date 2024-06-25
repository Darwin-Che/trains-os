#![no_std]
#![no_main]
#![feature(sync_unsafe_cell)]

use core::panic::PanicInfo;

use rust_pie::println;
use rust_pie::sys::log::Logger;
use rust_pie::log;

use rust_pie::sys::syscall::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::name_server::*;

use core::cell::SyncUnsafeCell;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    RpiUartRxResp(&'a mut RpiUartRxResp<'a>),
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start() {
    ker_create(2, b"PROGRAM\0rpi_uart\0ID\02\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_uart\0ID\00\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_bluetooth_commander\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_bluetooth_gatt\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_bluetooth_hci_rx\0").unwrap();

    // let mut recv_box: RecvBox = RecvBox::default();
    // let mut send_box: SendBox = SendBox::default();

    // let mut arr = [[0; 19]; 100];
    // let rx_tid = ker_create(2, b"PROGRAM\0rpi_uart\0ID\04\0").unwrap();

    // {
    //     let mut req = SendCtx::<RpiUartRxReq>::new(&mut send_box).unwrap();
    //     req.len = 19;
    // }

    // for i in 0..100 {
    //     ker_send(rx_tid, &send_box, &mut recv_box).unwrap();

    //     match RecvEnum::from_recv_bytes(&mut recv_box) {
    //         Some(RecvEnum::RpiUartRxResp(resp)) => {
    //             for j in 0..19 {
    //                 arr[i][j] = resp.bytes[j];
    //             }
    //         },
    //         None => println!("Rpi Uart Rx : Received None !"),
    //     };
    // }

    // for i in 0..100 {
    //     println!("{:X?}", arr[i]);
    // }

    // for i in 0..100 {
    //     log!("COPY {:X?}", arr[i]);
    // }
}