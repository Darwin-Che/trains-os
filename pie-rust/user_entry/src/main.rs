#![no_std]
#![no_main]

use core::ffi::{c_char, CStr};
use core::panic::PanicInfo;

#[link(name = "syscall")]
extern "C" {
    /*
    void uart_puts_ntm(size_t spiChannel, size_t uartChannel, const char *buf);
     */
    // fn uart_puts_ntm(spiChannel: usize, uartChannel: usize, buf: *const c_char);

    // fn uart_test();

    fn ke_print_raw(msg: *const c_char);
}

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn _start(x: i32) -> ! {
    let msg = CStr::from_bytes_until_nul(b"RUST TASK!!!\r\n\0").unwrap();

    unsafe {
        ke_print_raw(msg.as_ptr());
    }

    // unsafe {
    //     uart_puts_ntm(0, 0, 0 as *const c_char);
    // }
    loop {}
}
