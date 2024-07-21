pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;
pub use crate::api::clock::TICK_MS;

// PID_DT = 20 ms or 2 ticks
pub const PID_DT: f64 = 0.02;
pub const PID_DT_TICK: u64 = 20 / TICK_MS;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct PidTrigger {
}
