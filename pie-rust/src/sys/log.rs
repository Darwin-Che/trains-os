use core::fmt;
use core::fmt::Write;
use core::ffi::{c_char, c_int};
use crate::sys::syscall::*;
use crate::sys::types::*;
use crate::api::name_server::*;
use crate::api::rpi_uart::*;
use crate::println;

use lazy_static::lazy_static;
use core::cell::SyncUnsafeCell;
use heapless::String;

#[macro_export]
macro_rules! log_helper {
    ($($arg:tt)*) => ($crate::sys::log::_log(format_args!($($arg)*)););
}

#[macro_export]
macro_rules! log {
    () => ($crate::log_helper!("\r\n"));
    ($($arg:tt)*) => ($crate::log_helper!("{}\r\n", format_args!($($arg)*)));
}

pub struct Logger {
    logger_tid: Tid,
    send_box: SendBox<2048>,
    recv_box: RecvBox,
    buffer: String<2000>,
}

// lazy_static! {
//     static ref GLOBAL_LOGGER: SyncUnsafeCell<Logger> = SyncUnsafeCell::new(Logger::new());
// }

static GLOBAL_LOGGER: SyncUnsafeCell<Option<Logger>> = SyncUnsafeCell::new(None);

impl Logger {
    pub fn new() -> Self {
        let tid = ns_get("rpi_uart_2");
        Self {
            logger_tid: tid.unwrap(),
            send_box: SendBox::new(),
            recv_box: RecvBox::new(),
            buffer: String::new(),
        }
    }

    pub fn write_fmt(&mut self, args: fmt::Arguments) -> fmt::Result {
        self.buffer.clear();
        self.buffer.write_fmt(args)?;

        {
            let mut tx_req = SendCtx::<RpiUartTxBlockingReq>::new(&mut self.send_box).unwrap();
            tx_req.bytes = tx_req.attach_array(self.buffer.len()).unwrap();
            tx_req.bytes.copy_from_slice(self.buffer.as_bytes());
        }

        ker_send(self.logger_tid, &self.send_box, &mut self.recv_box).unwrap();
        Ok(())
    }
}

pub fn _log(args: fmt::Arguments) {
    let logger : &mut Option<Logger> = unsafe { &mut *GLOBAL_LOGGER.get() };
    if logger.is_none() {
        *logger = Some(Logger::new());
    }
    logger.as_mut().unwrap().write_fmt(args).unwrap();
}