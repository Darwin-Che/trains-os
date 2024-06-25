#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::log;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;

const DEBUG: bool = false;

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
    RpiUartRxResp(&'a mut RpiUartRxResp<'a>),
}

struct State {
    uart_0: Tid,
    send_box: SendBox,
    recv_box: RecvBox,
}

impl State {
    fn new() -> Self {
        Self {
            uart_0: ns_get("rpi_uart_0").unwrap(),
            send_box: SendBox::new(),
            recv_box: RecvBox::new(),
        }
    }

    fn read_n_bytes<const N: usize>(&mut self) -> [u8; N] {
        let mut data = [0; N];
        
        let mut uart_req = SendCtx::<RpiUartRxReq>::new(&mut self.send_box).unwrap();
        uart_req.len = N as u32;

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut self.recv_box) {
            Some(RecvEnum::RpiUartRxResp(uart_resp)) => {
                if DEBUG {
                    log!("rpi_bluetooth::read_n_bytes() Return 0x{:X?}", uart_resp.bytes);
                }
                data.copy_from_slice(&uart_resp.bytes);
            },
            _ => panic!("rpi_bluetooth::read_one_byte() Unexpected Response"),
        }

        data
    }

    fn read_one_byte(&mut self) -> u8 {
        let data: [u8; 1] = self.read_n_bytes();
        data[0]
    }

    fn read_bytes<F>(&mut self, len: usize, mut callback: F) where F: FnMut(&[u8]) {
        let mut uart_req = SendCtx::<RpiUartRxReq>::new(&mut self.send_box).unwrap();
        uart_req.len = len as u32;

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap(); 

        match RecvEnum::from_recv_bytes(&mut self.recv_box) {
            Some(RecvEnum::RpiUartRxResp(uart_resp)) => {
                if DEBUG {
                    log!("rpi_bluetooth::read_bytes() Start Callback");
                }
                callback(&uart_resp.bytes);
            },
            _ => panic!("rpi_bluetooth::read_bytes() Unexpected Response"),
        } 
    }
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start(ptr: *const c_char, len: usize) {
    let args = EntryArgs::new(ptr, len);
    args.print();

    ns_set("rpi_bluetooth_hci_rx").unwrap();

    let mut state = State::new();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let commander = ns_get("rpi_bluetooth_commander").unwrap();
    let gatt = ns_get("rpi_bluetooth_gatt").unwrap();

    loop {
        let hci_packet_id = state.read_one_byte();
        match hci_packet_id {
            // ACL Packet
            0x02 => {
                let header: [u8; 4] = state.read_n_bytes();
                let data_len = (header[3] as u16) << 8 | (header[2] as u16);

                state.read_bytes(data_len as usize, |data_bytes| {
                    log!("[ACL Recv] {:?} {:?}", header, data_bytes);

                    // Check for L2CAP Packet
                    let pdu_len = (data_bytes[1] as u16) << 8 | (data_bytes[0] as u16);
                    if pdu_len + 4 == data_len {
                        let chnl_id = (data_bytes[3] as u16) << 8 | (data_bytes[2] as u16);

                        let mut packet = SendCtx::<L2capPacket>::new(&mut send_box).unwrap();
                        packet.data = packet.attach_array(pdu_len as usize).unwrap();
                        packet.data.copy_from_slice(&data_bytes[4..]);
                        packet.chnl_id = chnl_id;
                        packet.acl_header[0] = header[0];
                        packet.acl_header[1] = header[1];

                        ker_send(gatt, &send_box, &mut recv_box).unwrap();
                    } else {
                        // let mut packet = SendCtx::<AclPacket>::new(&mut send_box).unwrap();
                        // packet.data = packet.attach_array(data_len as usize).unwrap();
                        // packet.data.copy_from_slice(data_bytes);
                    }
                });
            },
            // Event Packet
            0x04 => {
                let event_code = state.read_one_byte();
                let param_bytes_len = state.read_one_byte();

                state.read_bytes(param_bytes_len as usize, |param_bytes| {
                    match event_code {
                        // BLE Event
                        0x3e => { 
                            let subevent_code = param_bytes[0];
                            match subevent_code {
                                // LE Connection Complete
                                0x01 => {
                                    log!("[HCI Event] LE Connection Complete");
                                    let mut event = SendCtx::<HciLeConnectionComplete>::new(&mut send_box).unwrap();
                                    event.status = param_bytes[1];
                                    event.connection_handle = (param_bytes[3] as u16) << 8 | (param_bytes[2] as u16);
                                    event.role = param_bytes[4];
                                    event.peer_address_type = param_bytes[5];
                                    event.peer_address.copy_from_slice(&param_bytes[6..12]);
                                    event.connection_interval = (param_bytes[13] as u16) << 8 | (param_bytes[12] as u16);
                                    event.peripheral_latency = (param_bytes[15] as u16) << 8 | (param_bytes[14] as u16);
                                    event.supervision_timeout = (param_bytes[17] as u16) << 8 | (param_bytes[16] as u16);
                                    event.central_clock_accuracy = param_bytes[18];

                                    ker_send(commander, &send_box, &mut recv_box).unwrap();
                                },
                                _ => {
                                    log!("Bluetooth LE Unrecognized Subevent Code {} {:?}", subevent_code, param_bytes);
                                }
                            }
                        },
                        // Command Complete Event
                        0x0e => {
                            log!("[HCI Event] Command Complete");

                            let mut event = SendCtx::<HciCommandComplete>::new(&mut send_box).unwrap();
                            event.num_hci_command_packets = param_bytes[0];
                            event.command_opcode = (param_bytes[2] as u16) << 8 | (param_bytes[1] as u16);
                            event.return_param = event.attach_array(param_bytes.len() - 3).unwrap();
                            event.return_param.copy_from_slice(&param_bytes[3..]);

                            ker_send(commander, &send_box, &mut recv_box).unwrap();
                        },
                        // Disconnection Event
                        0x05 => {
                            log!("[HCI Event] Disconnected");

                            let mut event = SendCtx::<HciDisconnectionComplete>::new(&mut send_box).unwrap();
                            event.reason = param_bytes[3];

                            ker_send(commander, &send_box, &mut recv_box).unwrap();
                        },
                        // Command Status Event
                        0x0f => {
                            log!("[HCI Event] Command Status");

                            let mut event = SendCtx::<HciCommandStatus>::new(&mut send_box).unwrap();
                            event.status = param_bytes[0];
                            event.num_hci_command_packets = param_bytes[1];
                            event.command_opcode = (param_bytes[3] as u16) << 8 | (param_bytes[2] as u16);

                            ker_send(commander, &send_box, &mut recv_box).unwrap();
                        },
                        _ => {
                            log!("Bluetooth Unrecognized Event Code {} {:?}", event_code, param_bytes);
                        }
                    }
                });
            },
            _ => {
                panic!("Bluetooth Unrecognized packet id {}", hci_packet_id);
            }
        }
    }
}
