#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::log;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::api::name_server::*;
use rust_pie::api::clock::*;
use rust_pie::api::motor::*;
use libm;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const PWM_RANGE: i32 = 128;

struct Motor {
    pub pwm: RpiPwm,
    pub velo: i32,
    pub pin1: u32,
    pub pin2: u32,
}

impl Motor {
    pub fn new(pwm_pin: u32, pin1: u32, pin2: u32) -> Self {
        unsafe {
            setup_gpio(pin1, GPIO_SETTING_OUTPUT, GPIO_RESISTOR_PDP);
            setup_gpio(pin2, GPIO_SETTING_OUTPUT, GPIO_RESISTOR_PDP);
        }
        Self {
            pwm: RpiPwm::new(pwm_pin, PWM_RANGE.abs() as u32),
            velo: 0,
            pin1: pin1,
            pin2: pin2,
        }
    }

    pub fn update(&mut self, mut v: f64) {
        if v.is_nan() {
            return;
        }

        v = v.min(PWM_RANGE as f64).max(-PWM_RANGE as f64);
        
        let new_velo = libm::round(v) as i32;

        // Need to reset direction
        if self.velo * new_velo <= 0 {
            unsafe {
                if new_velo > 0 {
                    set_outpin_gpio(self.pin1);
                    clear_outpin_gpio(self.pin2);
                } else if new_velo < 0 {
                    clear_outpin_gpio(self.pin1);
                    set_outpin_gpio(self.pin2);
                } else {
                    clear_outpin_gpio(self.pin1);
                    clear_outpin_gpio(self.pin2);
                }
            }
        }

        // Set the speed
        self.pwm.set(new_velo.abs() as u32);

        self.velo = new_velo;
    }
}

struct State {
    pub left: Motor,
    pub right: Motor,
}

impl State {
    pub fn new() -> Self {
        Self {
            left: Motor::new(18, 14, 15),
            right: Motor::new(13, 19, 26),
        }
    }
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    MotorReq(&'a mut MotorReq),
}

#[no_mangle]
pub extern "C" fn _start() {
    // ker_create(3, b"PROGRAM\0motor_server_watchdog\0").unwrap();

    ns_set("motor_server").unwrap();

    RpiPwm::global_setup();

    let mut state = State::new();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::MotorReq(motor_req)) => {
                if let Some(v) = motor_req.left {
                    state.left.update(v);
                }
                if let Some(v) = motor_req.right {
                    state.right.update(v);
                }

                let mut motor_resp = SendCtx::<MotorResp>::new(&mut send_box).unwrap();
                motor_resp.left = state.left.velo;
                motor_resp.right = state.right.velo;
                ker_reply(sender_tid, &send_box).unwrap();
            },
            _ => {
                log!("[MOTOR] Unexpected Receive");
            }
        }
    }
}