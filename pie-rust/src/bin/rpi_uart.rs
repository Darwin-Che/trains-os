#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::println;
use rust_pie::api::rpi_uart::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::api::name_server;
use rust_pie::sys::entry_args::*;

use heapless::Deque;
use heapless::String;

const DEBUG: [u32; 0] = [];

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    ker_exit();
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    RpiUartRxReq(&'a mut RpiUartRxReq),
    RpiUartTxReq(&'a mut RpiUartTxReq<'a>),
    RpiUartTxBlockingReq(&'a mut RpiUartTxBlockingReq<'a>),
    RpiUartIntr(&'a mut RpiUartIntr),
}

#[derive(Debug)]
struct RxReq {
    pub tid: Tid,
    pub len: usize,
}

#[derive(Debug)]
struct TxReq {
    pub tid: Tid,
    pub end_at: u64,
}

struct State {
    uart_id: u32,
    reply_box: SendBox,
    rpi_uart: &'static mut RpiUart,
    tid_intr: Tid,

    // RxState
    rx_queue: Deque::<RxReq, 128>,
    rx_buf: Deque::<u8, 2048>,

    // TxState
    tx_buf: Deque::<u8, 2048>,

    // TxBlocked
    tx_queue: Deque::<TxReq, 128>,
    tx_idx: u64,
}

impl State {
    pub fn new(uart_id: u32, baudrate: u64) -> Self {
        let rpi_uart = RpiUart::new(uart_id, baudrate);
        rpi_uart.drain();

        Self {
            uart_id: uart_id,
            reply_box: SendBox::new(),
            rpi_uart: rpi_uart,
            tid_intr: ker_create(2, b"PROGRAM\0rpi_uart_intr_broker\0").unwrap(),
            rx_queue: Deque::new(),
            rx_buf: Deque::new(),
            tx_buf: Deque::new(),
            tx_queue: Deque::new(),
            tx_idx: 0,
        }
    }

    pub fn handle_rx_req(&mut self, tid: Tid, len: u32) {
        if DEBUG.contains(&self.uart_id) {
            println!("rpi_uart {} Rx Req tid={} len={}", self.uart_id, tid, len);
        }

        self.rx_queue.push_back(RxReq{ tid: tid, len: len as usize }).unwrap();

        self.read();
    }

    pub fn handle_tx_req(&mut self, tid: Tid, bytes: &[u8], blocking: bool) {
        if DEBUG.contains(&self.uart_id) {
            println!("rpi_uart {} Tx Req tid={} len={} blocking={}", self.uart_id, tid, bytes.len(), blocking);
        }

        for b in bytes {
            self.tx_buf.push_back(*b).unwrap();
        }

        if blocking {
            self.tx_queue.push_back(TxReq{ tid: tid, end_at: self.tx_idx + self.tx_buf.len() as u64}).unwrap();
        } else {
            // reply
            SendCtx::<RpiUartTxResp>::new(&mut self.reply_box).unwrap();
            ker_reply(tid, &self.reply_box).unwrap();
        }

        self.write();
    }

    pub fn write(&mut self) {
        if DEBUG.contains(&self.uart_id) {
            println!("rpi_uart {} Tx Write", self.uart_id);
        }

        while self.tx_buf.len() != 0 {
            let ch = *self.tx_buf.front().unwrap();
            if self.rpi_uart.putc(ch).is_some() {
                self.tx_buf.pop_front().unwrap();

                self.tx_idx += 1;
                
                if self.tx_queue.front().is_some() && self.tx_queue.front().unwrap().end_at == self.tx_idx {
                    if DEBUG.contains(&self.uart_id) {
                        println!("rpi_uart {} Tx unblocks tx_idx={} tx_buf_len={} tx_queue_len={}",
                                self.uart_id,
                                self.tx_idx, self.tx_buf.len(), self.tx_queue.len());
                    }
                    let front = self.tx_queue.pop_front().unwrap();
                    SendCtx::<RpiUartTxResp>::new(&mut self.reply_box).unwrap();
                    ker_reply(front.tid, &self.reply_box).unwrap(); 
                }
            } else {
                if DEBUG.contains(&self.uart_id) {
                    println!("rpi_uart {} Arm Tx at tx_idx={} tx_buf_len={} tx_queue_len={}", self.uart_id, self.tx_idx, self.tx_buf.len(), self.tx_queue.len());
                }
                self.rpi_uart.arm_tx();
                break;
            }
        }
    }

    pub fn read(&mut self) {
        if DEBUG.contains(&self.uart_id) {
            println!("rpi_uart {} Rx Read", self.uart_id);
        }

        // Do stuff on rpi_uart
        while let Some(ch) = self.rpi_uart.getc() {
            self.rx_buf.push_back(ch).unwrap();
        }

        // While there is some request and some data
        while !self.rx_queue.is_empty() && !self.rx_buf.is_empty() {
            // When the existing data is not enough for the next request
            if self.rx_queue.front().unwrap().len > self.rx_buf.len() {
                break;
            }

            // Reply the next request
            let req = self.rx_queue.pop_front().unwrap();
            {
                let mut resp = SendCtx::<RpiUartRxResp>::new(&mut self.reply_box).unwrap();
                resp.bytes = resp.attach_array(req.len).unwrap();
                // Fill in the data
                for i in 0..req.len {
                    resp.bytes[i] = self.rx_buf.pop_front().unwrap();
                }
            }
            if DEBUG.contains(&self.uart_id) {
                println!("rpi_uart {} Rx Finished req from tid={}", self.uart_id, req.tid);
            }
            ker_reply(req.tid, &self.reply_box).unwrap();
        }

        if DEBUG.contains(&self.uart_id) {
            println!("rpi_uart {} Rx Paused rx_buf_len={}", self.uart_id, self.rx_buf.len());
        }
    }

    pub fn rearm(&mut self) {
        // Rearm the intr
        let mut resp = SendCtx::<RpiUartIntr>::new(&mut self.reply_box).unwrap();
        resp.uart_id = self.uart_id;
        resp.mis = 0;
        ker_reply(self.tid_intr, &self.reply_box).unwrap();

        // Always arm the Rx
        self.rpi_uart.arm_rx();
    }
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start(ptr: *const c_char, len: usize) {
    let args = EntryArgs::new(ptr, len);
    args.print();

    let uart_id_str = args.get("ID").unwrap();
    let uart_id: u32 = uart_id_str.parse().unwrap();

    let mut name: String<32> = String::new();
    name.push_str("rpi_uart_").unwrap();
    name.push_str(uart_id_str).unwrap();
    name_server::ns_set(name.as_str()).unwrap();

    let mut state = State::new(uart_id, 115200);

    let mut recv_box: RecvBox = RecvBox::default();
    
    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::RpiUartRxReq(rx_req)) => state.handle_rx_req(sender_tid, rx_req.len),
            Some(RecvEnum::RpiUartTxReq(tx_req)) => state.handle_tx_req(sender_tid, &tx_req.bytes, false),
            Some(RecvEnum::RpiUartTxBlockingReq(tx_req)) => state.handle_tx_req(sender_tid, &tx_req.bytes, true),
            Some(RecvEnum::RpiUartIntr(intr)) => {
                if DEBUG.contains(&uart_id) {
                    println!("rpi_uart {} Intr {:X?}", intr.uart_id, intr.mis);
                }
                state.rearm();
                state.read();
                state.write();
            },
            None => println!("Rpi Uart Rx : Received None !"),
        };
    }
}