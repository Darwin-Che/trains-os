#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::gatt::*;
use rust_pie::api::clock::*;
use rust_pie::api::name_server::*;
use rust_pie::api::imu::*;
use rust_pie::api::motor::*;
use rust_pie::log;
use rust_pie::bog;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::api::pid::*;
use rust_pie::api::encoder::*;

use core::str;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const DEBUG: bool = false;

#[derive(Default)]
struct PidPitch {
    // Pid Param
    pub pid_pitch_p: f64,
    pub pid_pitch_d: f64,

    // Target
    pub t_pitch: f64,
    pub t_pitch_vel: f64,

    // Sensor
    pub s_pitch: f64,
    pub s_pitch_vel: f64,

    // Output
    pub o_pitch: f64,
}

impl PidPitch {
    pub fn new(pid_pitch_p: f64, pid_pitch_d: f64) -> Self {
        Self {
            pid_pitch_p,
            pid_pitch_d,
            ..Self::default()
        }
    }

    pub fn calc(&mut self) {
        self.o_pitch =
            (self.t_pitch - self.s_pitch) * self.pid_pitch_p
            + (self.t_pitch_vel - self.s_pitch_vel) * self.pid_pitch_d;
    }
}

#[derive(Default)]
struct PidSpeed {
    // Pid Param
    pub pid_speed_p: f64,
    pub pid_speed_i: f64,

    // Target
    pub t_speed: f64,

    // Sensor
    pub s_encoder_left: f64,
    pub s_encoder_right: f64,

    pub s_speed: f64,
    pub s_position: f64,

    // Output
    pub o_speed: f64,
}

impl PidSpeed {
    pub fn new(pid_speed_p: f64, pid_speed_i: f64) -> Self {
        Self {
            pid_speed_p,
            pid_speed_i,
            ..Self::default()
        }
    }

    pub fn calc(&mut self) {
        let encoder = (self.s_encoder_left + self.s_encoder_right) * 0.5;

        self.s_speed = encoder * 0.3 + self.s_speed * 0.7;

        let speed_delta = self.t_speed - self.s_speed;

        self.s_position += speed_delta;

        // We can linearize this change in the future
        self.o_speed = speed_delta * self.pid_speed_p + self.s_position * self.pid_speed_i;
    }
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    ImuResp(&'a mut ImuResp),
    EncoderResp(&'a mut EncoderResp),
}

#[no_mangle]
pub extern "C" fn _start() {
    let imu_server_tid = ns_get_wait("imu_server");
    let encoder_server_tid = ns_get_wait("encoder_server");

    let pid_trigger_tid = ker_create(PRIO_PID, b"PROGRAM\0pid_trigger\0").unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut pid_pitch = PidPitch::new(0.0, 0.0);
    let mut pid_speed = PidSpeed::new(0.0, 0.0);

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        if sender_tid == pid_trigger_tid {
            SendCtx::<PidTrigger>::new(&mut send_box).unwrap();
            ker_reply(sender_tid, &send_box).unwrap();

            SendCtx::<ImuReq>::new(&mut send_box).unwrap();
            ker_send(imu_server_tid, &send_box, &mut recv_box).unwrap();
            if let RecvEnum::ImuResp(imu_resp) = RecvEnum::from_recv_bytes(&mut recv_box).unwrap() {
                let new_pitch = (imu_resp.pitch as f64) * 0.01;
                let old_pitch = pid_pitch.s_pitch;
                pid_pitch.s_pitch_vel = (new_pitch - old_pitch) / PID_DT;
                pid_pitch.s_pitch = new_pitch;
            } else {
                log!("[PID] Unexpected Response from imu_server");
            }

            pid_pitch.calc();

            SendCtx::<EncoderReq>::new(&mut send_box).unwrap();
            ker_send(encoder_server_tid, &send_box, &mut recv_box).unwrap();
            if let RecvEnum::EncoderResp(encoder_resp) = RecvEnum::from_recv_bytes(&mut recv_box).unwrap() {
                pid_speed.s_encoder_left = encoder_resp.left / PID_DT;
                pid_speed.s_encoder_right = encoder_resp.right / PID_DT;
            } else {
                log!("[PID] Unexpected Response from encoder_server");
            }

            pid_speed.calc();

            // log!("[PID] output {} {}", pid_pitch.s_pitch, pid_speed.s_speed);

            continue;
        }
    }
}