pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct RpiUartRxReq {
    pub len: u32,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct RpiUartRxResp<'a> {
    pub bytes: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct RpiUartTxReq<'a> {
    pub bytes: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct RpiUartTxBlockingReq<'a> {
    pub bytes: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct RpiUartTxResp {
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct RpiUartIntr {
    pub uart_id: u32,
    pub mis: u32,
}