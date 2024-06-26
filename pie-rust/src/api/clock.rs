pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ClockNotifier {
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ClockWaitReq {
    pub ticks: u64,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ClockWaitResp {
}
