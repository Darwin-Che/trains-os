#![no_std]
#![no_main]

use core::panic::PanicInfo;
use rust_pie::api::name_server::*;
use rust_pie::api::clock::*;
use rust_pie::log;
use rust_pie::println;
use rust_pie::sys::entry_args::*;
use rust_pie::sys::syscall::*;
use rust_pie::sys::rpi::*;

use heapless::binary_heap::{BinaryHeap, Min};

const DEBUG: bool = false;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    log!("{}", info);
    ker_exit();
    loop {}
}

const QUANTUM: u64 = 10 * 1000; // us

#[derive(Debug, RecvEnumTrait)]
enum RecvEnum<'a> {
    ClockNotifier(&'a mut ClockNotifier),
    ClockWaitReq(&'a mut ClockWaitReq),
    ClockCurTickReq(&'a mut ClockCurTickReq),
}

#[derive(Debug, Eq, Ord, PartialEq, PartialOrd)]
struct WaitReq {
    pub until: u64,
    pub tid: Tid,
}

#[no_mangle]
pub extern "C" fn _start(_ptr: *const c_char, _len: usize) {
    ns_set("clock_server").unwrap();
    let mut notifier = ker_create(0, b"PROGRAM\0clock_notifier\0").unwrap();

    let clock = RpiClock::new();
    let mut t = clock.cur_u64();
    clock.intr_clear();

    if DEBUG {
        println!("[Clock] Start {}", t);
    }

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    let mut heap : BinaryHeap<WaitReq, Min, 1024> = BinaryHeap::new();
    let mut curtick : u64 = 0;

    loop {
        let sender = ker_recv(&mut recv_box);

        match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::ClockNotifier(_)) => {
                if DEBUG {
                    println!("[Clock] Wake from Interrupt");
                }

                SendCtx::<ClockNotifier>::new(&mut send_box).unwrap();
                ker_reply(sender, &send_box).unwrap();

                // Set the interrupt
                t += QUANTUM;
                clock.intr_arm(t);

                // Wake up any tasks
                curtick += 1;
                
                while heap.peek().is_some_and(|wait_req| wait_req.until <= curtick) {
                    let wait_req = heap.pop().unwrap();
                    
                    SendCtx::<ClockWaitResp>::new(&mut send_box).unwrap();
                    ker_reply(wait_req.tid, &send_box).unwrap();
                }

                if DEBUG {
                    println!("[Clock] Set Interrupt {}", clock.cur_u64());
                }
            },
            // Wait Request
            Some(RecvEnum::ClockWaitReq(cw)) => {
                if cw.ticks == 0 {
                    if DEBUG {
                        println!("[Clock] Wait Req NoWait {}", sender);
                    }
                    SendCtx::<ClockWaitResp>::new(&mut send_box).unwrap();
                    ker_reply(sender, &send_box).unwrap(); 
                } else {
                    if DEBUG {
                        println!("[Clock] Wait Req {}", sender);
                    }
                    heap.push(WaitReq { until: curtick + cw.ticks, tid: sender }).unwrap();
                }
            },
            // CurTick Request
            Some(RecvEnum::ClockCurTickReq(_)) => {
                let mut resp = SendCtx::<ClockCurTickResp>::new(&mut send_box).unwrap();
                resp.cur_tick = curtick;
                ker_reply(sender, &send_box).unwrap();
            },
            _ => {
                log!("[Clock] Unexpected Message");
            }
        }
    }
}