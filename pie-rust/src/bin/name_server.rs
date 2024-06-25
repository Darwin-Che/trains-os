#![no_std]
#![no_main]

use core::panic::PanicInfo;
use core::hash::{Hash, Hasher};

use rust_pie::println;
use rust_pie::api::name_server::*;

use heapless::FnvIndexMap;
use fnv::FnvHasher;

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

const DEBUG: bool = false;
const MAP_CAP : usize = 128;
type NameMap = FnvIndexMap::<u64, Tid, MAP_CAP>;

#[derive(Debug, RecvEnumTrait)]
enum RecvEnum<'a> {
    NsGetReq(&'a mut NsGetReq<'a>),
    NsSetReq(&'a mut NsSetReq<'a>),
}

/* Entry Function */

#[no_mangle]
pub extern "C" fn _start() {
    let mut name_map = NameMap::new();

    let mut send_box: SendBox = SendBox::default();
    let mut recv_box: RecvBox = RecvBox::default();

    if DEBUG {
        println!("Name Server : I am HERE!");
    }
    
    loop {
        let sender_tid = ker_recv(&mut recv_box);

        if DEBUG {
            println!("Name Server : ker_recv {}", sender_tid);
            println!("{:?}", &recv_box.recv_buf[0..32]);
            println!("{:?}", &recv_box.recv_buf[32..64]);
        }

        let mut ns_resp = SendCtx::<NsResp>::new(&mut send_box).unwrap();

        ns_resp.tid = match RecvEnum::from_recv_bytes(&mut recv_box) {
            Some(RecvEnum::NsGetReq(get_req)) => handle_get_req(&mut name_map, get_req),
            Some(RecvEnum::NsSetReq(set_req)) => handle_set_req(&mut name_map, set_req, sender_tid),
            None => { println!("Name Server : Received None !"); None },
        };

        if DEBUG {
            println!("Name Server : ker_reply {} {:?}", sender_tid, ns_resp.tid);
        }
        ker_reply(sender_tid, &send_box).unwrap();
    }
}

fn calc_hash_u8_slice(t: &[u8]) -> u64 {
    if DEBUG {
        println!("calc_hash_u8_slice : Start");
    }
    let mut s = FnvHasher::default();
    if DEBUG {
        println!("calc_hash_u8_slice : Default Hasher");
    }
    u8::hash_slice(t, &mut s);
    if DEBUG {
        println!("calc_hash_u8_slice : Hash Slice");
    }
    s.finish()
}

fn handle_get_req(name_map: &mut NameMap, get_req: &mut NsGetReq) -> Option<Tid> {
    if DEBUG {
        println!("Name Server : handle_get_req");
    }

    let array_bytes : &[u8] = &get_req.name;

    if DEBUG {
        println!("Name Server : array_bytes {:?}", array_bytes);
    }

    let hash_result : u64 = calc_hash_u8_slice(array_bytes);

    if DEBUG {
        println!("Name Server : name_map.get {hash_result}");
    }

    if let Some(tid) = name_map.get(&hash_result) {
        if *tid > 0 {
            return Some(*tid);
        }
    }

    return None;
}

fn handle_set_req(name_map: &mut NameMap, set_req: &mut NsSetReq, sender_tid: Tid) -> Option<Tid> {
    if DEBUG {
        println!("Name Server : handle_set_req");
    }

    let array_bytes : &[u8] = &set_req.name;

    if DEBUG {
        println!("Name Server : array_bytes {:?}", array_bytes);
    }

    let hash_result : u64 = calc_hash_u8_slice(array_bytes);

    if DEBUG {
        println!("Name Server : name_map.get {hash_result}");
    }

    if let Some(_tid) = name_map.get(&hash_result) {
        if DEBUG {
            println!("Name Server : Hash Collision Detected for {:?} !!!", array_bytes);
        }
        return None;
    }

    if DEBUG {
        println!("Name Server : name_map.insert {hash_result}");
    }

    name_map.insert(hash_result, sender_tid).unwrap();
    return Some(sender_tid);
}
