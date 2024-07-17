#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::log;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::api::imu::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::sys::helper::read_u16;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    RpiUartRxResp(&'a mut RpiUartRxResp<'a>),
}

#[no_mangle]
pub extern "C" fn _start() {
    let ps0 = 10;
    let ps1 = 11;
    unsafe {
        setup_gpio(ps0, GPIO_SETTING_OUTPUT, GPIO_RESISTOR_PUP);
        setup_gpio(ps1, GPIO_SETTING_OUTPUT, GPIO_RESISTOR_PDP);
    }

    let parent_tid = ker_parent_tid();
    let uart_tid = ker_create(PRIO_UART, b"PROGRAM\0rpi_uart\0ID\04\0").unwrap();

    let mut send_box_uart: SendBox = SendBox::default();
    let mut send_box_imu: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    // Find the 0xAA which is the start of a message
    'outer: loop {
        let mut req = SendCtx::<RpiUartRxReq>::new(&mut send_box_uart).unwrap();
        req.len = 20;
    
        let mut offset = None;
        ker_send(uart_tid, &send_box_uart, &mut recv_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::RpiUartRxResp(resp)) => {
                // Find offset
                for i in 0..resp.bytes.len()-1 {
                    if resp.bytes[i] == 0xAA && resp.bytes[i+1] == 0xAA {
                        offset = Some(i);
                        break;
                    }
                }
            },
            None => log!("[IMU Collector] Received None (outer)!"),
        };

        if let Some(x) = offset {
            let mut req = SendCtx::<RpiUartRxReq>::new(&mut send_box_uart).unwrap();
            if x == 0 {
                req.len = 18;
            } else {
                req.len = (x - 1) as u32;
            }
            ker_send(uart_tid, &send_box_uart, &mut recv_box).unwrap();
        } else {
            log!("[IMU Collector] Failed to find offset!")
        }

        // Now we are aligned with the boundary of the messages

        let mut req = SendCtx::<RpiUartRxReq>::new(&mut send_box_uart).unwrap();
        req.len = 19;

        loop {
            ker_send(uart_tid, &send_box_uart, &mut recv_box).unwrap();

            match RecvEnum::from_recv_bytes(&mut recv_box) {
                Some(RecvEnum::RpiUartRxResp(resp)) => {
                    let mut imu_raw = SendCtx::<ImuRawResp>::new(&mut send_box_imu).unwrap();
                    if resp.bytes[0] != 0xAA || resp.bytes[1] != 0xAA {
                        continue 'outer;
                    }
                    imu_raw.yaw = read_u16(&resp.bytes[3..]);
                    imu_raw.pitch = read_u16(&resp.bytes[5..]);
                    imu_raw.roll = read_u16(&resp.bytes[7..]);
                    imu_raw.x_accel = read_u16(&resp.bytes[9..]);
                    imu_raw.y_accel = read_u16(&resp.bytes[11..]);
                    imu_raw.z_accel = read_u16(&resp.bytes[13..]);
                    ker_send(parent_tid, &send_box_imu, &mut recv_box).unwrap();
                },
                None => panic!("[IMU Collector] Received None (inner)!"),
            };
        }
    }

    

    
}