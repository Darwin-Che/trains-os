use core::fmt;
use core::fmt::Write;
use core::ffi::{c_char, c_int};
use crate::sys::syscall::*;
use crate::sys::types::*;
use crate::api::name_server::*;
use crate::api::rpi_uart::*;

use lazy_static::lazy_static;
use core::cell::UnsafeCell;

#[macro_export]
macro_rules! log_helper {
    ($logger:ident $($arg:tt)*) => ($logger.write_fmt(format_args!($($arg)*)).unwrap(););
}

#[macro_export]
macro_rules! log {
    ($logger:ident) => ($crate::log_helper!($logger, "\r\n"));
    ($logger:ident $($arg:tt)*) => ($crate::log_helper!($logger, "{}\r\n", format_args!($($arg)*)));
}

pub struct Logger {
    logger_tid: Tid,
    send_box: SendBox<2048>,
    recv_box: RecvBox,
}



impl Logger {
    pub fn new() -> Self {
        Self {
            logger_tid: ns_get("rpi_uart_logger").unwrap(),
            send_box: SendBox::new(),
            recv_box: RecvBox::new()
        }
    }

    fn write_fmt(&mut self, args: fmt::Arguments) -> fmt::Result {
        {
            let mut tx_req = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
            (tx_req.bytes).write_fmt(args)?;
        }

        ker_send(self.logger_tid, &self.send_box, &mut self.recv_box).unwrap();
        Ok(())
    }
}