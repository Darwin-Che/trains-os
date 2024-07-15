use core::fmt;
use core::fmt::Write;
use crate::sys::types::*;
use crate::api::name_server::*;
use crate::api::gatt::*;
use crate::api::clock::*;

use core::cell::SyncUnsafeCell;
use heapless::String;

#[macro_export]
macro_rules! bog_helper {
    ($($arg:tt)*) => ($crate::sys::bog::_bog(format_args!($($arg)*)););
}

#[macro_export]
macro_rules! bog {
    () => ($crate::bog_helper!("\r\n"));
    ($($arg:tt)*) => ($crate::bog_helper!("{}\r\n", format_args!($($arg)*)));
}

pub struct Bogger {
    gatt_server_tid: Tid,
    send_box: SendBox<2048>,
    recv_box: RecvBox,
    buffer: String<2000>,
}

static GLOBAL_BOGGER: SyncUnsafeCell<Option<Bogger>> = SyncUnsafeCell::new(None);

const NAME_MSG: &[u8] = b"Msg";

impl Bogger {
    pub fn new() -> Self {
        let tid = ns_get_wait("rpi_bluetooth_gatt");
        Self {
            gatt_server_tid: tid,
            send_box: SendBox::new(),
            recv_box: RecvBox::new(),
            buffer: String::new(),
        }
    }

    pub fn write_fmt(&mut self, args: fmt::Arguments) -> fmt::Result {
        self.buffer.clear();
        self.buffer.write_fmt(args)?;

        {
            let mut bog_req = SendCtx::<GattServerPublishReq>::new(&mut self.send_box).unwrap();
            bog_req.name = bog_req.attach_array(NAME_MSG.len()).unwrap();
            bog_req.name.copy_from_slice(NAME_MSG);
            bog_req.bytes = bog_req.attach_array(self.buffer.len() + 8).unwrap();
            bog_req.bytes[0..8].copy_from_slice(&get_cur_tick().to_le_bytes());
            bog_req.bytes[8..].copy_from_slice(self.buffer.as_bytes());
        }

        ker_send(self.gatt_server_tid, &self.send_box, &mut self.recv_box).unwrap();
        Ok(())
    }
}

pub fn _bog(args: fmt::Arguments) {
    let bogger : &mut Option<Bogger> = unsafe { &mut *GLOBAL_BOGGER.get() };
    if bogger.is_none() {
        *bogger = Some(Bogger::new());
    }
    bogger.as_mut().unwrap().write_fmt(args).unwrap();
}