#![no_std]
#![no_main]

use core::panic::PanicInfo;

use rust_pie::println;

use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::RpiUart;
use rust_pie::api::rpi_uart::*;

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
    let rx_tid = ker_create(2, b"PROGRAM\0rpi_uart_rx\0ID\04\0").unwrap();

    let mut arr = [[0; 19]; 100];
    let mut recv_box: RecvBox = RecvBox::default();
    let mut send_box: SendBox = SendBox::default();

    {
        let mut req = SendCtx::<RpiUartRxReq>::new(&mut send_box).unwrap();
        req.len = 19;
    }

    for i in 0..100 {
        ker_send(rx_tid, &send_box, &mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::RpiUartRxResp(resp)) => {
                for j in 0..19 {
                    arr[i][j] = resp.bytes[j];
                }
            },
            None => println!("Rpi Uart Rx : Received None !"),
        }; 
    }

    for i in 0..100 {
        println!("{:X?}", arr[i]);
    }
}