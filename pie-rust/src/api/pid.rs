pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;
pub use crate::api::clock::TICK_MS;

use heapless::String;

// PID_DT = 20 ms
pub const PID_DT_S: f64 = 0.01;
pub const PID_DT_MS: u64 = 10;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct PidTrigger {
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct PidTuneReq<'a> {
    pub key: AttachedArray<'a, u8>,
    pub val: f64,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct PidTuneResp {
    pub pid_pitch_p: f64,
    pub pid_pitch_d: f64,
    pub pid_speed_p: f64,
    pub pid_speed_i: f64,
}