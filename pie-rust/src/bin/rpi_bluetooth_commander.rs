#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::rpi_uart::*;
use rust_pie::api::rpi_bluetooth::*;
use rust_pie::log;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;
use rust_pie::api::clock::*;

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const FIRMWARE: &'static [u8] = include_bytes!("BCM4345C0.hcd");

struct Commander {
    uart_0: Tid,
    send_box: SendBox,
    recv_box: RecvBox,
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    HciCommandComplete(&'a mut HciCommandComplete<'a>),
    HciLeConnectionComplete(&'a mut HciLeConnectionComplete),
    HciDisconnectionComplete(&'a mut HciDisconnectionComplete),
}

const OGF_HOST_CONTROL: u16 = 0x03;
const OGF_LE_CONTROL: u16 = 0x08;
const OGF_VENDOR: u16 = 0x3f;

const COMMAND_SET_BDADDR: u16 = 0x01;
const COMMAND_RESET_CHIP: u16 = 0x03;
const COMMAND_SET_BAUD: u16 = 0x18;
const COMMAND_LOAD_FIRMWARE: u16 = 0x2e;

const HCI_COMMAND_PKT: u8 = 0x01;

const COMMAND_COMPLETE_CODE: u16 = 0x0e;
const CONNECT_COMPLETE_CODE: u16 = 0x0f;

const LL_SCAN_ACTIVE: u8 = 0x01;
const LL_ADV_NONCONN_IND: u8 = 0x03;
const LL_ADV_CONN_IND: u8 = 0x00;

impl Commander {
    fn new() -> Self {
        Self {
            uart_0: ns_get("rpi_uart_0").unwrap(),
            send_box: SendBox::new(),
            recv_box: RecvBox::new(),
        }
    }

    fn start_active_advertising(&mut self) {
        let interval_min = (100 * 1000 / 625) as u16; // every 100ms
        let interval_max = (100 * 1000 / 625) as u16; // every 100ms
 
        self.set_le_advert_params(LL_ADV_CONN_IND, interval_min, interval_max, 0, 0);
        log!("[BT_CMD]::set_le_advert_params() Set advert_params Finished");
        self.set_le_advert_data();
        log!("[BT_CMD]::set_le_advert_data() Set advert_data Finished");
        self.set_le_advert_enable(1);
        log!("[BT_CMD]::set_le_advert_enable() Enable advert Finished");
    }

    fn stop_active_advertising(&mut self) {
        self.set_le_advert_enable(0);
    }

    fn set_le_advert_enable(&mut self, state: u8) {
        let command: [u8; 1] = [state];
        self.cmd(OGF_LE_CONTROL << 10 | 0x0a, &command);
        self.wait_command_complete(OGF_LE_CONTROL << 10 | 0x0a, &mut []);
    }

    fn set_le_advert_data(&mut self) {
        let command: [u8; 32] = [
            0x11,
            0x0c, 0x09, b'B', b'a', b'l', b'a', b'n', b'c', b'e', b'-', b'c', b'a', b'r',
            0x03, 0x03, 0xbc, 0xbc,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        ];
        self.cmd(OGF_LE_CONTROL << 10 | 0x08, &command);
        self.wait_command_complete(OGF_LE_CONTROL << 10 | 0x08, &mut []);
    }

    fn set_le_advert_params(&mut self, advert_type: u8, interval_min: u16, interval_max: u16, own_address_type: u8, filter_policy: u8) {
        let mut command: [u8; 15] = [0; 15];
        command[0] = (interval_min & 0xff) as u8;
        command[1] = (interval_min >> 8) as u8;
        command[2] = (interval_max & 0xff) as u8;
        command[3] = (interval_max >> 8) as u8;
        command[4] = advert_type;
        command[5] = own_address_type;
        command[13] = 0x07;
        command[14] = filter_policy;
        self.cmd(OGF_LE_CONTROL << 10 | 0x06, &command);
        self.wait_command_complete(OGF_LE_CONTROL << 10 | 0x06, &mut []);
    }

    fn set_le_eventmask(&mut self, mask: u8) {
        let mut command: [u8; 8] = [0; 8];
        command[0] = mask;
        self.cmd(OGF_LE_CONTROL << 10 | 0x01, &command);
        self.wait_command_complete(OGF_LE_CONTROL << 10 | 0x01, &mut []);
    }

    fn set_baud(&mut self) {
        let command: [u8; 6] = [0, 0, 0x00, 0xc2, 0x01, 0x00]; // little endian, 115200
        self.cmd(OGF_VENDOR << 10 | COMMAND_SET_BAUD, &command);
        self.wait_command_complete(OGF_VENDOR << 10 | COMMAND_SET_BAUD, &mut []);
    }

    fn set_addr(&mut self) {
        let command: [u8; 6] = [0xca, 0xca, 0xca, 0xca, 0xca, 0xca];
        self.cmd(OGF_VENDOR << 10 | COMMAND_SET_BDADDR, &command);
        self.wait_command_complete(OGF_VENDOR << 10 | COMMAND_SET_BDADDR, &mut []);
    }

    fn get_addr(&mut self) -> [u8; 6] {
        let mut buf: [u8; 7] = [0; 7];
        self.cmd(0x1009, &[]);
        self.wait_command_complete(0x1009, &mut buf);

        let mut addr: [u8; 6] = [0; 6];
        addr.copy_from_slice(&buf[1..]);

        addr
    }

    fn set_user_friendly_name(&mut self, name: &str) {
        // Allows at most 63 bytes for name
        let mut command = [0u8; 64];
        let mut len = name.len();
        if len > 63 {
            len = 63;
        }
        command[0..len].copy_from_slice(&name.as_bytes()[0..len]);
        self.cmd(OGF_HOST_CONTROL << 10 | 0x13, &command[0..len+1]);
        self.wait_command_complete(OGF_HOST_CONTROL << 10 | 0x13, &mut []);
    }

    fn cmd(&mut self, opcode: u16, data: &[u8]) {
        let opcode_lo: u8 = (opcode & 0xff) as u8;
        let opcode_hi: u8 = ((opcode & 0xff00) >> 8) as u8;

        let command_len = 4 + data.len();
        
        let mut uart_req = SendCtx::<RpiUartTxBlockingReq>::new(&mut self.send_box).unwrap();
        uart_req.bytes = uart_req.attach_array(command_len).unwrap();
        uart_req.bytes[0] = HCI_COMMAND_PKT;
        uart_req.bytes[1] = opcode_lo;
        uart_req.bytes[2] = opcode_hi;
        uart_req.bytes[3] = data.len() as u8;
        uart_req.bytes[4..].copy_from_slice(data);

        if DEBUG {
            log!("[BT_CMD]::hci_command_bytes() Send len={}", uart_req.bytes.len());
        }

        ker_send(self.uart_0, &self.send_box, &mut self.recv_box).unwrap();
    }

    fn wait_command_complete(&mut self, opcode: u16, data: &mut [u8]) {
        let tid = ker_recv(&mut self.recv_box);
        SendCtx::<HciReply>::new(&mut self.send_box).unwrap();
        ker_reply(tid, &self.send_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut self.recv_box) {
            Some(RecvEnum::HciCommandComplete(cc)) => {
                data.copy_from_slice(&cc.return_param[0..data.len()]);
                assert!(cc.command_opcode == opcode, "[BT_CMD]::wait_command_complete failed [opcode {} != {}]", cc.command_opcode, opcode);
                assert!(cc.num_hci_command_packets != 0, "[BT_CMD]::wait_command_complete num_hci_command_packets = 0");
            }
            _ => {
                panic!("[BT_CMD]::wait_command_complete failed [recv]");
            }
        }
    }

    fn wait_connection_complete(&mut self) -> HciLeConnectionComplete {
        let tid = ker_recv(&mut self.recv_box);
        SendCtx::<HciReply>::new(&mut self.send_box).unwrap();
        ker_reply(tid, &self.send_box).unwrap();

        match RecvEnum::from_recv_bytes(&mut self.recv_box) {
            Some(RecvEnum::HciLeConnectionComplete(cc)) => {
                return *cc;
            }
            _ => {
                panic!("[BT_CMD]::wait_command_complete failed [recv]");
            }
        } 
    }
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start(ptr: *const c_char, len: usize) {
    let args = EntryArgs::new(ptr, len);
    args.print();

    ns_set("rpi_bluetooth_commander").unwrap();

    let mut commander = Commander::new();

    // RESET
    commander.cmd(OGF_HOST_CONTROL << 10 | COMMAND_RESET_CHIP, &[]);
    commander.wait_command_complete(OGF_HOST_CONTROL << 10 | COMMAND_RESET_CHIP, &mut []);
    log!("[BT_CMD] Reset finished");

    // LOAD FIRMWARE
    log!("[BT_CMD] Loading Firmware Please Wait ...");
    commander.cmd(OGF_VENDOR << 10 | COMMAND_LOAD_FIRMWARE, &[]);
    commander.wait_command_complete(OGF_VENDOR << 10 | COMMAND_LOAD_FIRMWARE, &mut []);
    let mut idx = 0;
    while idx < FIRMWARE.len() {
        // log!("[BT_CMD]::bt_load_firmware() idx={}", idx);
        let opcode_lo = FIRMWARE[idx] as u16;
        let opcode_hi = FIRMWARE[idx+1] as u16;
        let opcode: u16 = (opcode_hi << 8) | opcode_lo;
        let data_len = FIRMWARE[idx+2] as usize;
        commander.cmd(opcode, &FIRMWARE[idx+3..idx+3+data_len]);
        commander.wait_command_complete(opcode, &mut []);
        idx += 3 + data_len;
    } 
    log!("[BT_CMD] Load Firmware finished");

    // Let's Wait 2 second
    wait_ticks(200);

    // SET BAUDRATE
    commander.set_baud();
    log!("[BT_CMD] Set Baudrate finished");

    // SET ADDR
    commander.set_addr();
    log!("[BT_CMD] Set Addr finished");

    // GET ADDR
    let addr = commander.get_addr();
    log!("[BT_CMD] Get Addr {:?}", addr);

    commander.set_le_eventmask(0xff);
    log!("[BT_CMD] Set eventmask Finished");

    commander.set_user_friendly_name("balance_car");

    commander.start_active_advertising();
    log!("[BT_CMD] Start active_advertising Finished");

    let conn = commander.wait_connection_complete();
    log!("[BT_CMD] Connection Complete {:?}", conn);

    commander.stop_active_advertising();
    log!("[BT_CMD] Stop active_advertising Finished");

    let mut recv_box: RecvBox = RecvBox::default();
    let mut send_box: SendBox = SendBox::default();

    log!("[BT_CMD] Enter Loop");

    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::HciDisconnectionComplete(discc)) => {
                log!("[BT_CMD] Disconnection Detected");

                SendCtx::<HciReply>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap(); 

                commander.start_active_advertising();
                log!("[BT_CMD] Start active_advertising Finished");
            
                let conn = commander.wait_connection_complete();
                log!("[BT_CMD] Connection Complete {:?}", conn);
            
                commander.stop_active_advertising();
                log!("[BT_CMD] Stop active_advertising Finished");
            },
            _ => {
                SendCtx::<HciReply>::new(&mut send_box).unwrap();
                ker_reply(sender_tid, &send_box).unwrap(); 
                log!("Commander Cannot Handle Command");
            }
        }
    }
}