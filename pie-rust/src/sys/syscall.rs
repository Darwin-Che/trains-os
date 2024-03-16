use core::ffi::{c_char, c_int, c_size_t};

#[link(name = "syscall")]
extern "C" {
    // extern int ke_create(int priority, void (*function)(), const char * args, size_t args_len);
    fn ke_create(priority: c_int, args: *const c_char, args_len: c_size_t) -> c_int;
}

pub fn ker_create(priority: i32, args: &[u8]) -> i32 {
    unsafe {
        ke_create(
            priority as c_int,
            args.as_ptr() as *const c_char,
            args.len()
        )
    }
}