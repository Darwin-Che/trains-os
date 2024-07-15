use core::fmt;
use core::fmt::Write;
use crate::sys::types::*;
use crate::api::name_server::*;
use crate::api::rpi_uart::*;

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

#[macro_export]
macro_rules! log_helper_blocking {
    ($($arg:tt)*) => ($crate::sys::log::_log_blocking(format_args!($($arg)*)););
}

#[macro_export]
macro_rules! logblk {
    () => ($crate::log_helper_blocking!("\r\n"));
    ($($arg:tt)*) => ($crate::log_helper_blocking!("{}\r\n", format_args!($($arg)*)));
}

pub struct Logger {
    logger_tid: Tid,
    send_box: SendBox<2048>,
    recv_box: RecvBox,
    buffer: String<2000>,
}

static GLOBAL_LOGGER: SyncUnsafeCell<Option<Logger>> = SyncUnsafeCell::new(None);

impl Logger {
    pub fn new() -> Self {
        let tid = ns_get_wait("rpi_uart_2");
        Self {
            logger_tid: tid,
            send_box: SendBox::new(),
            recv_box: RecvBox::new(),
            buffer: String::new(),
        }
    }

    pub fn write_fmt(&mut self, args: fmt::Arguments) -> fmt::Result {
        self.buffer.clear();
        self.buffer.write_fmt(args)?;

        let mut tx_req = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        tx_req.bytes = tx_req.attach_array(self.buffer.len()).unwrap();
        tx_req.bytes.copy_from_slice(self.buffer.as_bytes());

        ker_send(self.logger_tid, &self.send_box, &mut self.recv_box).unwrap();
        Ok(())
    }

    pub fn write_fmt_blocking(&mut self, args: fmt::Arguments) -> fmt::Result {
        self.buffer.clear();
        self.buffer.write_fmt(args)?;

        let mut tx_req = SendCtx::<RpiUartTxBlockingReq>::new(&mut self.send_box).unwrap();
        tx_req.bytes = tx_req.attach_array(self.buffer.len()).unwrap();
        tx_req.bytes.copy_from_slice(self.buffer.as_bytes());

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

pub fn _log_blocking(args: fmt::Arguments) {
    let logger : &mut Option<Logger> = unsafe { &mut *GLOBAL_LOGGER.get() };
    if logger.is_none() {
        *logger = Some(Logger::new());
    }
    logger.as_mut().unwrap().write_fmt_blocking(args).unwrap();
}