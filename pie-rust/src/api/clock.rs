pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;

use core::cell::SyncUnsafeCell;

pub const TICK_MS: u64 = 10;

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

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ClockCurTickReq {
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct ClockCurTickResp {
    pub cur_tick: u64,
}

// API

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    ClockCurTickResp(&'a mut ClockCurTickResp),
    ClockWaitResp(&'a mut ClockWaitResp),
}

pub struct ClockAPI {
    clock_server: Tid,
    get_cur_tick_send_box: SendBox,
    wait_ticks_send_box: SendBox,
    recv_box: RecvBox,
}

impl ClockAPI {
    pub fn new() -> Self {
        let tid = crate::api::name_server::ns_get("clock_server");
        let mut s = Self {
            clock_server: tid.unwrap(),
            get_cur_tick_send_box: SendBox::new(),
            wait_ticks_send_box: SendBox::new(),
            recv_box: RecvBox::new(),
        };

        SendCtx::<ClockCurTickReq>::new(&mut s.get_cur_tick_send_box).unwrap();

        s
    }

    pub fn get_cur_tick(&mut self) -> u64 {
        ker_send(self.clock_server, &self.get_cur_tick_send_box, &mut self.recv_box).unwrap();
        match RecvEnum::from_recv_bytes(&mut self.recv_box) {
            Some(RecvEnum::ClockCurTickResp(resp)) => {
                return resp.cur_tick;
            },
            _ => {
                panic!("get_cur_tick Unexpected Response");
            }
        };
    }

    pub fn wait_ticks(&mut self, ticks: u64) {
        let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut self.wait_ticks_send_box).unwrap();
        wait_req.ticks = ticks;
        ker_send(self.clock_server, &self.wait_ticks_send_box, &mut self.recv_box).unwrap();
        match RecvEnum::from_recv_bytes(&mut self.recv_box) {
            Some(RecvEnum::ClockWaitResp(_)) => {
                return;
            },
            _ => {
                panic!("wait_ticks Unexpected Response");
            }
        };
    }
}

static GLOBAL_CLOCK_API: SyncUnsafeCell<Option<ClockAPI>> = SyncUnsafeCell::new(None);

pub fn get_cur_tick() -> u64 {
    let clock_api : &mut Option<ClockAPI> = unsafe { &mut *GLOBAL_CLOCK_API.get() };
    if clock_api.is_none() {
        *clock_api = Some(ClockAPI::new());
    }
    clock_api.as_mut().unwrap().get_cur_tick()
}

pub fn wait_ticks(ticks: u64) {
    let clock_api : &mut Option<ClockAPI> = unsafe { &mut *GLOBAL_CLOCK_API.get() };
    if clock_api.is_none() {
        *clock_api = Some(ClockAPI::new());
    }
    clock_api.as_mut().unwrap().wait_ticks(ticks)
}