#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::log;
use rust_pie::sys::syscall::*;
use rust_pie::api::imu::*;
use rust_pie::api::name_server::*;
use rust_pie::api::gatt::*;

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
    ImuReq(&'a mut ImuReq),
    ImuRawResp(&'a mut ImuRawResp),
}

#[no_mangle]
pub extern "C" fn _start() {
    ker_create(3, b"PROGRAM\0imu_collector\0").unwrap();

    ns_set("imu_server").unwrap();

    let gatt_server_tid = ns_get_wait("rpi_bluetooth_gatt");

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut imu_cur = ImuResp {
        yaw: 0, pitch: 0, roll: 0, x_accel: 0, y_accel: 0, z_accel: 0,
    };

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::ImuReq(_)) => {
                let mut reply = SendCtx::<ImuResp>::new(&mut send_box).unwrap();
                **reply = imu_cur;
                ker_reply(sender_tid, &send_box).unwrap();
            },
            Some(RecvEnum::ImuRawResp(imu_raw)) => {
                SendCtx::<ImuRawReq>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap();
                imu_cur.yaw = imu_raw.yaw;
                imu_cur.pitch = imu_raw.pitch;
                imu_cur.roll = imu_raw.roll;
                imu_cur.x_accel = imu_raw.x_accel;
                imu_cur.y_accel = imu_raw.y_accel;
                imu_cur.z_accel = imu_raw.z_accel;

                let mut imu_update = SendCtx::<GattServerPublishReq>::new(&mut send_box).unwrap();
                imu_update.name = imu_update.attach_array(b"Imu".len()).unwrap();
                imu_update.name.copy_from_slice(b"Imu");
                imu_update.bytes = imu_update.attach_array(12).unwrap();
                imu_update.bytes.copy_from_slice(&imu_cur.to_bytes());

                ker_send(gatt_server_tid, &send_box, &mut recv_box).unwrap();
            },
            None => panic!("[IMU Server] Received None !"),
        }
    }
}