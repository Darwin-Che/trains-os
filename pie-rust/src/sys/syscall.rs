use core::ffi::{c_char, c_int, c_size_t};
use crate::sys::types::Tid;

#[link(name = "syscall")]
extern "C" {
    fn ke_create(priority: c_int, args: *const c_char, args_len: c_size_t) -> c_int;
    fn ke_my_parent_tid() -> c_int;
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
