#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::api::clock::*;
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
    HciLeConnectionComplete(&'a mut HciLeConnectionComplete),
    ClockWaitResp(&'a mut ClockWaitResp),
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

    pub fn resp_gatt(&mut self, handle: u16, chnl_id: u16, gatt_packet: &[u8]) {
        let gatt_len = gatt_packet.len();
        let acl_len = gatt_len + 4;
        let total_len = acl_len + 4 + 1;

        let mut packet = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        packet.bytes = packet.attach_array(total_len).unwrap();
        packet.bytes[0] = 0x02; // ACL
        packet.bytes[1] = (handle & 0xff) as u8;
        packet.bytes[2] = (handle >> 8) as u8;
        packet.bytes[3] = (acl_len & 0xff) as u8;
        packet.bytes[4] = (acl_len >> 8) as u8;
        packet.bytes[5] = (gatt_len & 0xff) as u8;
        packet.bytes[6] = (gatt_len >> 8) as u8;
        packet.bytes[7] = (chnl_id & 0xff) as u8;
        packet.bytes[8] = (chnl_id >> 8) as u8;
        packet.bytes[9..].copy_from_slice(gatt_packet);

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();
    }

    pub fn resp_acl_empty(&mut self, handle: u16) {
        let acl_len = 0;

        let mut packet = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        packet.bytes = packet.attach_array(5).unwrap();
        packet.bytes[0] = 0x02; // ACL
        packet.bytes[1] = (handle & 0xff) as u8;
        packet.bytes[2] = (handle >> 8) as u8;
        packet.bytes[3] = (acl_len & 0xff) as u8;
        packet.bytes[4] = (acl_len >> 8) as u8;

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();
    }
}

struct ACLState {
    pub conn: bool,
    pub handle: u16,
    pub last_packet_tick: u64,
}

struct L2CapState {
}

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    ns_set("rpi_bluetooth_gatt").unwrap();
    ker_create(2, b"PROGRAM\0clock_server_helper\0").unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut responder = Responder::new();

    let mut acl_state = ACLState { conn: false, handle: 0, last_packet_tick: 0 };

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::L2capPacket(packet)) => {
                SendCtx::<HciReply>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap();

                match packet.data[0] {
                    // ATT_EXCHANGE_MTU_REQ
                    0x02 => {
                        let client_mtu = (packet.data[2] as u16) << 8 | (packet.data[1] as u16);
                        log!("[GATT] ATT_EXCHANGE_MTU_REQ client MTU = {}", client_mtu);

                        responder.resp_gatt(acl_state.handle, packet.chnl_id, &[0x03, 5, 2])
                    },
                    // ATT_EXCHANGE_MTU_RESP
                    0x03 => {
                        let client_mtu = (packet.data[2] as u16) << 8 | (packet.data[1] as u16);
                        log!("[GATT] ATT_EXCHANGE_MTU_RESP client MTU = {}", client_mtu);
                    },
                    _ => {
                        log!("[GATT-L2CAP] Unknown {:?}", packet);
                    }
                }
                acl_state.last_packet_tick = get_cur_tick();
            },
            // Connection Complete Event
            Some(RecvEnum::HciLeConnectionComplete(cc)) => {
                SendCtx::<HciReply>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap();

                acl_state.conn = true;
                acl_state.handle = cc.connection_handle;
                acl_state.last_packet_tick = get_cur_tick();

                // responder.resp_gatt(acl_state.handle, 0x04, &[0x02, 15, 2])
            },
            // Check if Timeout happened
            Some(RecvEnum::ClockWaitResp(_)) => {
                if acl_state.conn {
                    let cur_tick = get_cur_tick();
                    if cur_tick - acl_state.last_packet_tick >= 5 {
                        // Timeout happened, send empty acl packet
                        // log!("[GATT] Wait finished, send empty acl packet");
                        responder.resp_acl_empty(acl_state.handle);

                        // Wait for more ticks
                        let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                        wait_req.ticks = 5;
                        ker_reply(sender_tid, &send_box);
                    } else {
                        // log!("[GATT] Wait finished, wait for more ticks");
                        // Wait for more ticks
                        let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                        wait_req.ticks = acl_state.last_packet_tick + 5 - cur_tick;
                        ker_reply(sender_tid, &send_box);
                    }
                } else {
                    // Wait for more ticks
                    let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                    wait_req.ticks = 5;
                    ker_reply(sender_tid, &send_box); 
                }
            }
            None => { log!("GATT : Received None !"); },
        };
    }
}