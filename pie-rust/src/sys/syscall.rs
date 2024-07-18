use core::ffi::{c_char, c_int, c_size_t};
use crate::sys::types::Tid;

#[link(name = "syscall")]
extern "C" {
    fn ke_create(priority: c_int, args: *const c_char, args_len: c_size_t) -> c_int;
    fn ke_my_parent_tid() -> c_int;
    fn ke_my_tid() -> c_int;
    fn ke_await_event(intr_id: u32) -> c_int;
    fn ke_exit();
    fn ke_sys_health(ptr: *mut KerSysHealth);
    fn ke_quadrature_encoder_init(pin_a: u32, pin_b: u32) -> c_int;
    fn ke_quadrature_encoder_get(id: c_int, stat: *mut KerQuadEncoderStat) -> c_int;
}

pub fn ker_exit() {
    unsafe {
        ke_exit();
    }
}

pub fn ker_create(priority: i32, args: &[u8]) -> Result<Tid, i32> {
    let ret = unsafe {
        ke_create(
            priority as c_int,
            args.as_ptr() as *const c_char,
            args.len()
        )
    };
    if ret > 0 {
        Ok(ret as Tid)
    } else {
        Err(ret as i32)
    }
}

pub fn ker_parent_tid() -> Tid {
    unsafe {
        ke_my_parent_tid() as Tid
    }
}
pub fn ker_my_tid() -> Tid {
    unsafe {
        ke_my_tid() as Tid
    }
}

// Return the status of the await event
pub fn ker_await_uart(uart_id: u32) -> Result<u32, i32> {
    unsafe {
        // 153 = 96 + 57
        let ret : i32 = ke_await_event((153 << 16) + uart_id);

        if ret < 0 {
            Err(ret)
        } else {
            Ok(ret as u32)
        }
    }
}

pub fn ker_await_clock() -> Result<(), i32> {
    unsafe {
        let ret : i32 = ke_await_event(97 << 16);

        if ret < 0 {
            Err(ret)
        } else {
            Ok(())
        }
    }
}

#[repr(C)]
pub struct KerSysHealth {
    pub idle_percent: i32,
}

pub fn ker_sys_health() -> KerSysHealth {
    let mut ret = KerSysHealth {idle_percent: 0};
    unsafe {
        ke_sys_health((&mut ret) as *mut KerSysHealth);
    }
    ret
}

pub fn ker_quadrature_encoder_init(pin_a: u32, pin_b: u32) -> Result<i32, i32> {
    unsafe {
        let ret : i32 = ke_quadrature_encoder_init(pin_a, pin_b);
        if ret < 0 {
            Err(ret)
        } else {
            Ok(ret)
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct KerQuadEncoderStat {
    pub forward_cnt: u64,
    pub backward_cnt: u64,
    pub invalid_1_cnt: u64,
    pub invalid_2_cnt: u64,
    pub debug: u64,
}

pub fn ker_quadrature_encoder_get(id: i32) -> KerQuadEncoderStat {
    let mut ret = KerQuadEncoderStat {forward_cnt: 0, backward_cnt: 0, invalid_1_cnt: 0, invalid_2_cnt: 0, debug: 0};
    unsafe {
        ke_quadrature_encoder_get(id, (&mut ret) as *mut KerQuadEncoderStat);
    }
    ret
}
