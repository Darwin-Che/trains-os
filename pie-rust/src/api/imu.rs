pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ImuReq {
}

#[repr(C)]
#[derive(Copy, Clone, Debug, Default, MsgTrait)]
pub struct ImuResp {
    pub yaw: u16,
    pub pitch: u16,
    pub roll: u16,
    pub x_accel: u16,
    pub y_accel: u16,
    pub z_accel: u16,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ImuRawReq {
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ImuRawResp {
    pub yaw: u16,
    pub pitch: u16,
    pub roll: u16,
    pub x_accel: u16,
    pub y_accel: u16,
    pub z_accel: u16,
}

impl ImuResp {
    pub fn to_bytes(&self) -> [u8; 12] {
        let mut ret = [0; 12];
        ret[0] = self.yaw as u8;
        ret[1] = (self.yaw >> 8) as u8;
        ret[2] = self.pitch as u8;
        ret[3] = (self.pitch >> 8) as u8;
        ret[4] = self.roll as u8;
        ret[5] = (self.roll >> 8) as u8;
        ret[6] = self.x_accel as u8;
        ret[7] = (self.x_accel >> 8) as u8;
        ret[8] = self.y_accel as u8;
        ret[9] = (self.y_accel >> 8) as u8;
        ret[10] = self.z_accel as u8;
        ret[11] = (self.z_accel >> 8) as u8;
        ret
    }
}