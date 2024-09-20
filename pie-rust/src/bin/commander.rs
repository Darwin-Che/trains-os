#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::api::clock::*;
use rust_pie::api::name_server::*;
use rust_pie::api::imu::*;
use rust_pie::api::motor::*;
use rust_pie::api::pid::*;
use rust_pie::log;
use rust_pie::bog;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;

use core::str;

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
    GattMonitorResp(&'a mut GattMonitorResp<'a>),
    ImuResp(&'a mut ImuResp),
    MotorResp(&'a mut MotorResp),
    PidTuneResp(&'a mut PidTuneResp),
}

const NAME_CMD_ACK: &[u8] = b"Cmd Ack";

#[no_mangle]
pub extern "C" fn _start() {
    let gatt_relay = ker_create(3, b"PROGRAM\0gatt_monitor_relay\0name\0Cmd Input\0").unwrap();
    let gatt_server = ns_get("rpi_bluetooth_gatt").unwrap();
    
    let mut send_box_monitor: SendBox = SendBox::default();
    SendCtx::<GattMonitorReq>::new(&mut send_box_monitor).unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();
    let mut recv_box_cmd: RecvBox = RecvBox::default();

    loop {
        ker_send(gatt_relay, &send_box_monitor, &mut recv_box_cmd).unwrap();

        match RecvEnum::from_recv_bytes(&mut recv_box_cmd) {
            Some(RecvEnum::GattMonitorResp(monitor_resp)) => {
                let id = if monitor_resp.bytes.len() >= 8 {
                    u64::from_le_bytes(monitor_resp.bytes[0..8].try_into().unwrap())
                } else {
                    0
                };

                let cmd = if monitor_resp.bytes.len() >= 8 {
                    str::from_utf8(&monitor_resp.bytes[8..]).unwrap()
                } else {
                    str::from_utf8(&monitor_resp.bytes[..]).unwrap()
                }; 

                log!("[CMD] {} {}", id, cmd);

                let mut cmd_ack = SendCtx::<GattServerPublishReq>::new(&mut send_box).unwrap();
                cmd_ack.name = cmd_ack.attach_array(NAME_CMD_ACK.len()).unwrap();
                cmd_ack.name.copy_from_slice(NAME_CMD_ACK);
                cmd_ack.bytes = cmd_ack.attach_array(16).unwrap();
                cmd_ack.bytes[0..8].copy_from_slice(&id.to_le_bytes());
                cmd_ack.bytes[8..16].copy_from_slice(&get_cur_tick().to_le_bytes());

                ker_send(gatt_server, &send_box, &mut recv_box).unwrap();

                if cmd == "imu" {
                    cmd_execute_imu(cmd);
                } else if cmd.starts_with("pwm") {
                    let velo = cmd
                        .strip_prefix("pwm").unwrap()
                        .trim()
                        .parse::<f64>().unwrap();

                    cmd_execute_pwm(velo);
                } else if cmd.starts_with("pid") {
                    cmd_execute_pid(
                        cmd
                        .strip_prefix("pid").unwrap()
                        .trim()
                    );
                }

            },
            _ => {
                panic!("[CMD] Unexpected RecvEnum");
            }
        }
    }
}

fn cmd_execute_imu(cmd: &str) {
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let imu_server_tid = ns_get("imu_server").unwrap();
    let imu_req = SendCtx::<ImuReq>::new(&mut send_box).unwrap();
    ker_send(imu_server_tid, &send_box, &mut recv_box).unwrap();
    match RecvEnum::from_recv_bytes(&mut recv_box) {
        Some(RecvEnum::ImuResp(imu_resp)) => {
            bog!("[CMD] imu {:?}", imu_resp);
        },
        _ => {
            panic!("[CMD execute] Unexpected RecvEnum");
        }
    }
}

fn cmd_execute_pwm(velo: f64) {
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let motor_server_tid = ns_get("motor_server").unwrap();
    let mut motor_req = SendCtx::<MotorReq>::new(&mut send_box).unwrap();
    motor_req.left = Some(velo);
    motor_req.right = Some(velo);
    ker_send(motor_server_tid, &send_box, &mut recv_box).unwrap();
    match RecvEnum::from_recv_bytes(&mut recv_box) {
        Some(RecvEnum::MotorResp(motor_resp)) => {
            bog!("[CMD] pwm left={} right={}", motor_resp.left, motor_resp.right);
        },
        _ => {
            panic!("[CMD execute] Unexpected RecvEnum");
        }
    }
}

fn cmd_execute_pid(cmd: &str) {
    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let pid_tid = ns_get("pid").unwrap();

    let mut args = cmd.split(' ');
    let key = args.next().unwrap();

    if key == "enable" || key == "disable" {
        let mut pid_enable = SendCtx::<PidEnable>::new(&mut send_box).unwrap();
        pid_enable.enabled = key == "enable";
        ker_send(pid_tid, &send_box, &mut recv_box).unwrap();
        return;
    }

    let val = args.next().unwrap().parse::<f64>().unwrap_or(0.0);

    let mut pid_tune = SendCtx::<PidTuneReq>::new(&mut send_box).unwrap();
    pid_tune.key = pid_tune.attach_array(key.len()).unwrap();
    pid_tune.key.copy_from_slice(key.as_bytes());
    pid_tune.val = val;

    ker_send(pid_tid, &send_box, &mut recv_box).unwrap();
    match RecvEnum::from_recv_bytes(&mut recv_box) {
        Some(RecvEnum::PidTuneResp(tune_resp)) => {
            bog!("[CMD] pid pitch_p={} pitch_d={} speed_p={} speed_i={}",
                tune_resp.pid_pitch_p, tune_resp.pid_pitch_d,
                tune_resp.pid_speed_p, tune_resp.pid_speed_i);
        },
        _ => {
            panic!("[CMD execute PID] Unexpected RecvEnum");
        }
    }
}