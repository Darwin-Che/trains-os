#![no_std]
#![no_main]
#![feature(sync_unsafe_cell)]

use core::panic::PanicInfo;
use rust_pie::log;
use rust_pie::sys::syscall::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::clock::*;
use rust_pie::sys::rpi::*;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    RpiUartRxResp(&'a mut RpiUartRxResp<'a>),
    ClockWaitResp(&'a mut ClockWaitResp),
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start() {
    ker_create(2, b"PROGRAM\0rpi_uart\0ID\02\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_uart\0ID\00\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_bluetooth_commander\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_bluetooth_gatt\0").unwrap();
    ker_create(2, b"PROGRAM\0rpi_bluetooth_hci_rx\0").unwrap();

    let tid_timeout = ker_create(2, b"PROGRAM\0clock_server_helper\0").unwrap();

    let mut recv_box: RecvBox = RecvBox::default();
    let mut send_box: SendBox = SendBox::default();

    let pwm_state = [0, 1, 0, 1];
    let a_state = [0, 1, 0, 0];
    let b_state = [0, 0, 0, 1];

    unsafe {
        setup_gpio(12, 0x01, 0x02); // PINOUT, PULLUP
        setup_gpio(20, 0x01, 0x02); // PINOUT, PULLUP
        setup_gpio(21, 0x01, 0x02); // PINOUT, PULLUP
    }

    loop {
        for i in 0..4 {
            // Wait 2 seconds
            let sender_tid = ker_recv(&mut recv_box);
            match RecvEnum::from_recv_bytes(&mut recv_box) {
                Some(RecvEnum::ClockWaitResp(_)) => {
                    let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                    wait_req.ticks = 200;
                    ker_reply(sender_tid, &send_box).unwrap();
                },
                _ => {
                    log!("[rpi] RecvEnum Unexpected!");
                }
            }

            log!("[rpi] set {} {} {}", pwm_state[i], a_state[i], b_state[i]);

            if pwm_state[i] == 0 {
                // clear the pin
                unsafe {
                    clear_outpin_gpio(12);
                }
            } else {
                // set the pin
                unsafe {
                    set_outpin_gpio(12);
                }
            }

            if a_state[i] == 0 {
                unsafe {
                    clear_outpin_gpio(20);
                } 
            } else {
                unsafe {
                    set_outpin_gpio(20);
                }
            }

            if b_state[i] == 0 {
                unsafe {
                    clear_outpin_gpio(21);
                } 
            } else {
                unsafe {
                    set_outpin_gpio(21);
                }
            }
        }        
    }

    // let clock_server = ns_get("clock_server").unwrap();
    // loop {
    //     let mut req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
    //     req.ticks = 10;
    //     ker_send(clock_server, &send_box, &mut recv_box);

    //     log!("[TEST] Clock Server Wakeup");
    // }

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