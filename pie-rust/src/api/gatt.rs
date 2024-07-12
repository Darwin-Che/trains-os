pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct GattMonitorReq {
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct GattMonitorResp<'a> {
    pub bytes: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct GattServerMonitorReq<'a> {
    pub name: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct GattServerMonitorResp<'a> {
    pub bytes: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct GattServerPublishReq<'a> {
    pub name: AttachedArray<'a, u8>,
    pub bytes: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct GattServerPublishResp {
}