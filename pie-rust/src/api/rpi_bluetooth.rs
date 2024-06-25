pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct HciCommandComplete<'a> {
    pub num_hci_command_packets: u8,
    pub command_opcode: u16,
    pub return_param: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct HciCommandStatus {
    pub num_hci_command_packets: u8,
    pub command_opcode: u16,
    pub status: u8,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait, Copy, Clone)]
pub struct HciLeConnectionComplete {
    pub status: u8,
    pub connection_handle: u16,
    pub role: u8,
    pub peer_address_type: u8,
    pub peer_address: [u8; 6],
    pub connection_interval: u16,
    pub peripheral_latency: u16,
    pub supervision_timeout: u16,
    pub central_clock_accuracy: u8,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait, Copy, Clone)]
pub struct HciDisconnectionComplete {
    pub reason: u8,
}

// ACL

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct AclPacket<'a> {
    pub data: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct L2capPacket<'a> {
    pub acl_header: [u8; 2],
    pub chnl_id: u16,
    pub data: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct HciReply {
}
