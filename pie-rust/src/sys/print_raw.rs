use core::fmt;
use core::fmt::Write;
use core::ffi::{c_char, c_int};

#[link(name = "syscall")]
extern "C" {
    fn ke_print_raw(msg: *const c_char, len: c_int);
}

pub fn ker_print_raw(msg: &str) {
    unsafe {
        ke_print_raw(
            msg.as_ptr() as *const c_char,
            msg.len() as c_int
        )
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::sys::print_raw::_print(format_args!($($arg)*)));
}

// #[macro_export]
// macro_rules! println {
//     () => ($crate::print!("\r\n"));
//     ($($arg:tt)*) => ($crate::print!("{}\r\n", format_args!($($arg)*)));
// }

#[macro_export]
macro_rules! println {
    ($($arg:tt)*) => ();
}

pub struct KerPrintRaw;

impl fmt::Write for KerPrintRaw {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        ker_print_raw(s);
        Ok(())
    }
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    let mut k = KerPrintRaw{};
    k.write_fmt(args).unwrap();
}