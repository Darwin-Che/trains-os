#![no_std]
#![no_main]
#![feature(sync_unsafe_cell)]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::api::clock::*;
use rust_pie::log;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::helper::*;
use rust_pie::sys::gatt::*;
use core::cell::SyncUnsafeCell;

use heapless::Vec;

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
    HciDisconnectionComplete(&'a mut HciDisconnectionComplete),
}

const ATT_ERROR_RSP: u8 = 0x01;
const GATT_CHNL_ID: u16 = 0x04;

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

    pub fn resp_gatt(&mut self, acl_handle: u16, gatt_packet: &[u8]) {
        let gatt_len = gatt_packet.len();
        let acl_len = gatt_len + 4;
        let total_len = acl_len + 4 + 1;

        let mut packet = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        packet.bytes = packet.attach_array(total_len).unwrap();
        packet.bytes[0] = 0x02; // ACL
        packet.bytes[1] = (acl_handle & 0xff) as u8;
        packet.bytes[2] = (acl_handle >> 8) as u8;
        packet.bytes[3] = (acl_len & 0xff) as u8;
        packet.bytes[4] = (acl_len >> 8) as u8;
        packet.bytes[5] = (gatt_len & 0xff) as u8;
        packet.bytes[6] = (gatt_len >> 8) as u8;
        packet.bytes[7] = (GATT_CHNL_ID & 0xff) as u8;
        packet.bytes[8] = (GATT_CHNL_ID >> 8) as u8;
        packet.bytes[9..].copy_from_slice(gatt_packet);

        log!("[GATT] Resp {:?}", &packet.bytes[..]);

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();
    }

    pub fn resp_acl_empty(&mut self, acl_handle: u16) {
        let acl_len = 0;

        let mut packet = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        packet.bytes = packet.attach_array(5).unwrap();
        packet.bytes[0] = 0x02; // ACL
        packet.bytes[1] = (acl_handle & 0xff) as u8;
        packet.bytes[2] = (acl_handle >> 8) as u8;
        packet.bytes[3] = (acl_len & 0xff) as u8;
        packet.bytes[4] = (acl_len >> 8) as u8;

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();
    }

    pub fn update_trigger(&mut self, acl_handle: u16, charac: &GattCharac) {
        if charac.client_config_handle.is_none() {
            return;
        }

        let client_config = global_gatt().att_read(charac.client_config_handle.unwrap()).unwrap().att_val[0];
        if client_config == 0 {
            return;
        }

        log!("[GATT] [update_trigger] {}", charac.name);

        let val_len = global_gatt().att_read(charac.value_handle).unwrap().att_val.len();
        let gatt_len = val_len + 3;
        let acl_len = gatt_len + 4;

        let mut packet = SendCtx::<RpiUartTxReq>::new(&mut self.send_box).unwrap();
        packet.bytes = packet.attach_array(val_len + 12).unwrap();
        packet.bytes[0] = 0x02; // ACL
        packet.bytes[1] = (acl_handle & 0xff) as u8;
        packet.bytes[2] = (acl_handle >> 8) as u8;
        packet.bytes[3] = (acl_len & 0xff) as u8;
        packet.bytes[4] = (acl_len >> 8) as u8;
        packet.bytes[5] = (gatt_len & 0xff) as u8;
        packet.bytes[6] = (gatt_len >> 8) as u8;
        packet.bytes[7] = (GATT_CHNL_ID & 0xff) as u8;
        packet.bytes[8] = (GATT_CHNL_ID >> 8) as u8;
        packet.bytes[9] =
            if client_config & CHARAC_CLIENT_CONFIG_NOTIFY != 0 {
                0x1b
            } else if client_config & CHARAC_CLIENT_CONFIG_INDICATE != 0 {
                0x1d
            } else {
                0x0
            }; // Notify or Indicate
        packet.bytes[10] = (charac.value_handle & 0xff) as u8;
        packet.bytes[11] = (charac.value_handle >> 8) as u8;
        packet.bytes[12..].copy_from_slice(
            &global_gatt().att_read(charac.value_handle).unwrap().att_val
        );
        
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

/* GATT */

static GLOBAL_GATT: SyncUnsafeCell<Option<Gatt>> = SyncUnsafeCell::new(None);

fn global_gatt() -> &'static mut Gatt {
    let gatt : &mut Option<Gatt> = unsafe { &mut *GLOBAL_GATT.get() };
    gatt.as_mut().unwrap()
}

fn global_gatt_set() {
    let gatt : &mut Option<Gatt> = unsafe { &mut *GLOBAL_GATT.get() };
    *gatt = Some(Gatt::new(&[
        (
            GattBuilderService{service_uuid: 0x1800u16.into()},
            &[
                // Device Name
                GattBuilderCharac{name: None, charac_uuid: 0x2a00u16.into(), property: CHARAC_PROPERTY_READ,
                    init_val: Some(Vec::from_slice("CAR".as_bytes()).unwrap())},
                // Appreance
                GattBuilderCharac{name: None, charac_uuid: 0x2a01u16.into(), property: CHARAC_PROPERTY_READ,
                    init_val: Some(Vec::from_slice(&[0x80, 0x01]).unwrap())},
            ]
        ),
        (
            GattBuilderService{service_uuid: 0x1801u16.into()},
            &[
                GattBuilderCharac{name: Some("ServiceChanged"), charac_uuid: 0x2a05u16.into(), property: CHARAC_PROPERTY_INDICATE,
                    init_val: Some(Vec::from_slice(&[0x00, 0x00, 0xff, 0xff]).unwrap())},
            ]
        ),
        (
            GattBuilderService{service_uuid: 0x0100u128.into()},
            &[
                GattBuilderCharac{name: Some("Clock"), charac_uuid: 0x0101u128.into(), property: CHARAC_PROPERTY_READ | CHARAC_PROPERTY_NOTIFY,
                    init_val: Some(Vec::from_slice(&0u64.to_le_bytes()).unwrap())},
                GattBuilderCharac{name: Some("CPU Usage"), charac_uuid: 0x0102u128.into(), property: CHARAC_PROPERTY_READ | CHARAC_PROPERTY_NOTIFY,
                    init_val: Some(Vec::from_slice(&0u64.to_le_bytes()).unwrap())},
            ]
        )
    ]));
}

/* LOCAL DATA */
struct LocalData {}

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    ns_set("rpi_bluetooth_gatt").unwrap();
    let tid_timeout = ker_create(2, b"PROGRAM\0clock_server_helper\0").unwrap();
    let tid_clock = ker_create(2, b"PROGRAM\0clock_server_helper\0").unwrap();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut responder = Responder::new();

    let mut acl_state = ACLState { conn: false, handle: 0, last_packet_tick: 0 };

    global_gatt_set();
    global_gatt().print_attr_vec();

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

                        responder.resp_gatt(acl_state.handle, &[0x03, 5, 2])
                    },
                    // ATT_EXCHANGE_MTU_RESP
                    0x03 => {
                        let client_mtu = (packet.data[2] as u16) << 8 | (packet.data[1] as u16);
                        log!("[GATT] ATT_EXCHANGE_MTU_RESP client MTU = {}", client_mtu);
                    },
                    // ATT_READ_BY_GROUP_TYPE_REQ
                    0x10 => {
                        let start_handle = read_u16(&packet.data[1..]);
                        let end_handle = read_u16(&packet.data[3..]);
                        let attr_group_type = packet.data[5..].try_into().unwrap();
                        log!("[GATT] [ATT_READ_BY_GROUP_TYPE_REQ] [{}, {}] by {}", start_handle, end_handle, attr_group_type);
                        let handle_arr = global_gatt().gatt_read_by_group_type([start_handle, end_handle], attr_group_type);
                        if handle_arr.len() == 0 {
                            responder.resp_gatt(acl_state.handle, &[
                                ATT_ERROR_RSP,
                                0x10, // ATT_READ_BY_GROUP_TYPE_REQ
                                (start_handle & 0xff) as u8, (start_handle >> 8) as u8,
                                0x0a,
                            ]);
                        } else {
                            let value_len = global_gatt().att_read(handle_arr[0]).unwrap().att_val.len() as u8;
                            let mut resp = Vec::<u8, 400>::from_slice(&[
                                0x11, // ATT_READ_BY_GROUP_TYPE_RSP
                                value_len + 2 + 2, // Attribute Length
                            ]).unwrap();
                            for handle_id in handle_arr {
                                resp.push((handle_id & 0xff) as u8).unwrap();
                                resp.push((handle_id >> 8) as u8).unwrap(); 

                                let group_end = global_gatt().gatt_read_end_of_group(handle_id).unwrap();
                                resp.push((group_end & 0xff) as u8).unwrap();
                                resp.push((group_end >> 8) as u8).unwrap();

                                let val = global_gatt().att_read(handle_id).unwrap();
                                resp.extend_from_slice(&val.att_val).unwrap();
                            }
                            responder.resp_gatt(acl_state.handle, &resp);
                        }
                    }
                    // ATT_READ_BY_TYPE_REQ
                    0x08 => {
                        let start_handle = read_u16(&packet.data[1..]);
                        let end_handle = read_u16(&packet.data[3..]); 
                        let attr_type = packet.data[5..].try_into().unwrap();

                        log!("[GATT] [ATT_READ_BY_TYPE_REQ] [{}, {}] by {}", start_handle, end_handle, attr_type);
                        let handle_arr = global_gatt().gatt_read_by_type([start_handle, end_handle], attr_type);
                        if handle_arr.len() == 0 {
                            responder.resp_gatt(acl_state.handle, &[
                                ATT_ERROR_RSP,
                                0x08, // ATT_READ_BY_GROUP_TYPE_REQ
                                (start_handle & 0xff) as u8, (start_handle >> 8) as u8,
                                0x0a,
                            ]);
                        } else {
                            let value_len = global_gatt().att_read(handle_arr[0]).unwrap().att_val.len() as u8;
                            let mut resp = Vec::<u8, 400>::from_slice(&[
                                0x09, // ATT_READ_BY_GROUP_TYPE_RSP
                                value_len + 2, // Attribute Length
                            ]).unwrap();
                            for handle_id in handle_arr {
                                resp.push((handle_id & 0xff) as u8).unwrap();
                                resp.push((handle_id >> 8) as u8).unwrap(); 

                                let val = global_gatt().att_read(handle_id).unwrap();
                                resp.extend_from_slice(&val.att_val).unwrap();
                            }
                            responder.resp_gatt(acl_state.handle, &resp);
                        }
                    },
                    // ATT_FIND_INFORMATION_RSP
                    0x04 => {
                        let start_handle = read_u16(&packet.data[1..]);
                        let end_handle = read_u16(&packet.data[3..]); 
                        log!("[GATT] [ATT_FIND_INFORMATION_RSP] [{}, {}]", start_handle, end_handle);
                        let handle_arr = global_gatt().gatt_find_info([start_handle, end_handle]);
                        if handle_arr.len() == 0 {
                            responder.resp_gatt(acl_state.handle, &[
                                ATT_ERROR_RSP,
                                0x04, // ATT_FIND_INFORMATION_RSP
                                (start_handle & 0xff) as u8, (start_handle >> 8) as u8,
                                0x0a,
                            ]);
                        } else {
                            let value_len = global_gatt().att_read(handle_arr[0]).unwrap().att_type.to_vec().len();
                            let mut resp = Vec::<u8, 400>::from_slice(&[
                                0x05, // ATT_FIND_INFORMATION_RSP
                                if value_len == 2 { 0x01 } else { 0x02 }, // Format
                            ]).unwrap();
                            for handle_id in handle_arr {
                                resp.push((handle_id & 0xff) as u8).unwrap();
                                resp.push((handle_id >> 8) as u8).unwrap(); 

                                let val = global_gatt().att_read(handle_id).unwrap();
                                resp.extend_from_slice(&val.att_type.to_vec()).unwrap();
                            }
                            responder.resp_gatt(acl_state.handle, &resp);
                        }
                    },
                    // ATT_WRITE_REQ
                    0x12 => {
                        let handle_id = read_u16(&packet.data[1..]);
                        log!("[GATT] [ATT_WRITE_REQ] {} {:?}", handle_id, &packet.data[3..]);
                        global_gatt().att_write(handle_id, &packet.data[3..]);
                        responder.resp_gatt(acl_state.handle, &[
                            0x13, // ATT_WRITE_RSP
                        ]);

                        // If it sets any notifications, we need to send the current value over
                        if let Some(charac) = global_gatt().get_charac_by_client_config_handle(handle_id) {
                            responder.update_trigger(acl_state.handle, &charac);
                        }
                    },
                    // ATT_HANDLE_VALUE_CFM
                    0x1e => {
                        log!("[GATT] [ATT_HANDLE_VALUE_CFM]");
                    },
                    // ATT_READ_REQ
                    0x0a => {
                        let handle_id = read_u16(&packet.data[1..]);
                        log!("[GATT] [ATT_READ_REQ] {}", handle_id);
                        if let Ok(attr) = global_gatt().att_read(handle_id) {
                            let mut resp = Vec::<_, 400>::from_slice(&[
                                0x0b, // ATT_READ_RSP
                            ]).unwrap();
                            resp.extend_from_slice(&attr.att_val).unwrap();
                            responder.resp_gatt(acl_state.handle, &resp);
                        } else {
                            responder.resp_gatt(acl_state.handle, &[
                                ATT_ERROR_RSP,
                                0x0a, // ATT_READ_REQ
                                (handle_id & 0xff) as u8, (handle_id >> 8) as u8,
                                0x01, // Reason: Invalid Handle
                            ]);
                        }
                    }
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
            // Disconnection Event
            Some(RecvEnum::HciDisconnectionComplete(dc)) => {
                SendCtx::<HciReply>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap(); 

                acl_state.conn = false;

                // Turn off any notification
                global_gatt().clear_subscription();
            }
            // Check if Timeout happened
            Some(RecvEnum::ClockWaitResp(_)) => {
                if sender_tid == tid_clock {
                    if acl_state.conn {
                        /* Clock */
                        let mut cur_tick = get_cur_tick();
                        let charac = global_gatt().charac_by_name("Clock").unwrap();
                        global_gatt().att_write(charac.value_handle, &cur_tick.to_le_bytes());
                        responder.update_trigger(acl_state.handle, &charac);

                        /* CPU Usage */
                        let charac = global_gatt().charac_by_name("CPU Usage").unwrap();
                        let sys_health = ker_sys_health();
                        global_gatt().att_write(charac.value_handle, &sys_health.idle_percent.to_le_bytes());
                        responder.update_trigger(acl_state.handle, &charac);
                    }
                    // Wait Until the next 1/2 second
                    let cur_tick = get_cur_tick();
                    let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                    wait_req.ticks = 50 - (cur_tick % 50);
                    ker_reply(sender_tid, &send_box).unwrap();
                }
                if sender_tid == tid_timeout {
                    if acl_state.conn {
                        let cur_tick = get_cur_tick();
                        if cur_tick - acl_state.last_packet_tick >= 5 {
                            // Timeout happened, send empty acl packet
                            // log!("[GATT] Wait finished, send empty acl packet");
                            responder.resp_acl_empty(acl_state.handle);

                            // Wait for more ticks
                            let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                            wait_req.ticks = 5;
                            ker_reply(sender_tid, &send_box).unwrap();
                        } else {
                            // log!("[GATT] Wait finished, wait for more ticks");
                            // Wait for more ticks
                            let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                            wait_req.ticks = acl_state.last_packet_tick + 5 - cur_tick;
                            ker_reply(sender_tid, &send_box).unwrap();
                        }
                    } else {
                        // Wait for more ticks
                        let mut wait_req = SendCtx::<ClockWaitReq>::new(&mut send_box).unwrap();
                        wait_req.ticks = 5;
                        ker_reply(sender_tid, &send_box).unwrap(); 
                    }
                }
            }
            None => { log!("GATT : Received None !"); },
        };
    }
}