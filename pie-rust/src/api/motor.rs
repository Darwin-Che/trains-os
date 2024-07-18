pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct MotorReq {
    pub left: Option<f64>,
    pub right: Option<f64>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct MotorResp {
    pub left: i32,
    pub right: i32,
}