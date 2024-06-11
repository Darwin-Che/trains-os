#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::println;
use rust_pie::api::rpi_uart::*;
use rust_pie::sys::syscall::*;

#[allow(dead_code)]
const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    RpiUartIntr(&'a mut RpiUartIntr),
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start() {
    let parent_tid = ker_parent_tid();

    let mut recv_box: RecvBox = RecvBox::default();
    let mut send_box: SendBox = SendBox::default();

    {
        let mut init_req = SendCtx::<RpiUartIntr>::new(&mut send_box).unwrap();
        init_req.uart_id = 0;
        init_req.mis = 0;
    }

    loop {
        if DEBUG {
            println!("rpi_uart_intr_broker send");
        }

        ker_send(parent_tid, &send_box, &mut recv_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::RpiUartIntr(ask)) => {
                let mut req = SendCtx::<RpiUartIntr>::new(&mut send_box).unwrap();
                req.uart_id = ask.uart_id;

                // Wait for UART Interrupt
                if DEBUG {
                    println!("rpi_uart_intr_broker start uart_id={}", ask.uart_id);
                }

                let status = ker_await_uart(ask.uart_id).unwrap();

                if DEBUG {
                    println!("rpi_uart_intr_broker end uart_id={} status={:X?}", ask.uart_id, status);
                }

                req.mis = status;
            },
            _ => panic!("Rpi Uart Intr Broker : Received Invalid Message From Parent"),
        }
    }
}
