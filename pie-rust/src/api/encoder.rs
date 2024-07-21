pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct EncoderReq {
}

#[repr(C)]
#[derive(Copy, Clone, Debug, Default, MsgTrait)]
pub struct EncoderResp {
    pub left: f64,
    pub right: f64,
}

impl EncoderResp{
    pub fn add(&mut self, other: &Self) {
        self.left += other.left;
        self.right += other.right;
    }
}