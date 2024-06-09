#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::println;
use rust_pie::api::rpi_uart::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;
use rust_pie::api::name_server;

use heapless::Deque;

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    RpiUartRxReq(&'a mut RpiUartRxReq),
    RpiUartIntr(&'a mut RpiUartIntr),
}

#[derive(Debug)]
struct RxReq {
    pub tid: Tid,
    pub len: usize,
}

struct RxState {
    uart_id: u32,
    reply_box: SendBox,
    req_queue: Deque::<RxReq, 128>,
    rx_buf: Deque::<u8, 1024>,
    rpi_uart: &'static mut RpiUart,
    tid_intr: Tid,
}

impl RxState {
    pub fn new(uart_id: u32, baudrate: u32) -> Self {
        Self {
            uart_id: uart_id,
            reply_box: SendBox::new(),
            req_queue: Deque::new(),
            rx_buf: Deque::new(),
            rpi_uart: RpiUart::new(uart_id, baudrate),
            tid_intr: ker_create(2, b"PROGRAM\0rpi_uart_intr_broker\0").unwrap(),
        }
    }

    pub fn handle_rx_req(&mut self, tid: Tid, len: u32) {
        if DEBUG {
            println!("rpi_uart_rx {} handle_rx_req tid={} len={}", self.uart_id, tid, len);
        }
        self.req_queue.push_back(RxReq{ tid: tid, len: len as usize }).unwrap();
        self.try_resolve();
    }

    pub fn handle_rx_intr(&mut self) {
        if DEBUG {
            println!("rpi_uart_rx {} handle_rx_intr", self.uart_id);
        }
        self.read();
        self.try_resolve();
    }

    pub fn read(&mut self) {
        if DEBUG {
            println!("rpi_uart_rx {} read", self.uart_id);
        }

        while let Some(ch) = self.rpi_uart.getc() {
            self.rx_buf.push_back(ch).unwrap();
        }

        // Rearm the intr
        let mut resp = SendCtx::<RpiUartIntr>::new(&mut self.reply_box).unwrap();
        resp.uart_id = self.uart_id;
        resp.tx = false;
        resp.rx = true;
        ker_reply(self.tid_intr, &self.reply_box).unwrap();
    }

    pub fn try_resolve(&mut self) {
        // While there is some request and some data
        while !self.req_queue.is_empty() && !self.rx_buf.is_empty() {
            // When the existing data is not enough for the next request
            if self.req_queue.front().unwrap().len > self.rx_buf.len() {
                return;
            }

            // Reply the next request
            let req = self.req_queue.pop_front().unwrap();
            {
                let mut resp = SendCtx::<RpiUartRxResp>::new(&mut self.reply_box).unwrap();
                resp.bytes = resp.attach_array(req.len).unwrap();
                // Fill in the data
                for i in 0..req.len {
                    resp.bytes[i] = self.rx_buf.pop_front().unwrap();
                }
            }
            if DEBUG {
                println!("rpi_uart_rx {} try_resolve tid={}", self.uart_id, req.tid);
            }
            ker_reply(req.tid, &self.reply_box).unwrap();
        }
    }
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start() {
    let uart_id = 4;

    name_server::ns_set("rpi_uart_rx_4");

    let mut state = RxState::new(uart_id, 115200);

    let mut recv_box: RecvBox = RecvBox::default();
    
    loop {
        let sender_tid = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::RpiUartRxReq(rx_req)) => state.handle_rx_req(sender_tid, rx_req.len),
            Some(RecvEnum::RpiUartIntr(_)) => state.handle_rx_intr(),
            None => println!("Rpi Uart Rx : Received None !"),
        };
    }
}