#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::log;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;

const DEBUG: bool = true;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    L2capPacket(&'a mut L2capPacket<'a>),
}

struct Responder {
    uart_0: Tid,
    send_box: SendBox,
    recv_box: RecvBox,
}

impl Responder {
    pub fn new() -> Self {
        Self {
            uart_0: ns_get("rpi_uart_0").unwrap(),
            send_box: SendBox::new(),
            recv_box: RecvBox::new(),
        }
    }

    pub fn resp_gatt(&mut self, acl_header: [u8; 2], chnl_id: u16, gatt_packet: &[u8]) {
        let gatt_len = gatt_packet.len();
        let acl_len = gatt_len + 4;
        let total_len = acl_len + 4 + 1;

        let mut packet = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        packet.bytes = packet.attach_array(total_len).unwrap();
        packet.bytes[0] = 0x02; // ACL
        packet.bytes[1] = acl_header[0];
        packet.bytes[2] = acl_header[1];
        packet.bytes[3] = (acl_len & 0xff) as u8;
        packet.bytes[4] = (acl_len >> 8) as u8;
        packet.bytes[5] = (gatt_len & 0xff) as u8;
        packet.bytes[6] = (gatt_len >> 8) as u8;
        packet.bytes[7] = (chnl_id & 0xff) as u8;
        packet.bytes[8] = (chnl_id >> 8) as u8;
        packet.bytes[9..].copy_from_slice(gatt_packet);

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();
    }
}

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    ns_set("rpi_bluetooth_gatt").unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut responder = Responder::new();

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        SendCtx::<HciReply>::new(&mut send_box).unwrap();
        ker_reply(sender_tid, &send_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::L2capPacket(packet)) => {
                match packet.data[0] {
                    // ATT_EXCHANGE_MTU_REQ | ATT_EXCHANGE_MTU_RESP
                    0x02 | 0x03 => {
                        let client_mtu = (packet.data[2] as u16) << 8 | (packet.data[1] as u16);
                        log!("[GATT] client MTU = {}", client_mtu);

                        responder.resp_gatt(packet.acl_header, packet.chnl_id, &[0x02, 247, 0])
                    },
                    _ => {
                        log!("[GATT-L2CAP] Unknown {:?}", packet);
                    }
                }
            },
            None => { log!("GATT : Received None !"); },
        };
    }
}