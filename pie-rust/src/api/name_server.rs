pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct NsGetReq<'a> {
    pub name: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct NsSetReq<'a> {
    pub tid: Tid,
    pub name: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct NsResp {
    pub tid: Option<Tid>,
}
