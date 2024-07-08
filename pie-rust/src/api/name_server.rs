pub use msgbox_macro::*;
pub use crate::sys::msgbox::*;
pub use crate::sys::types::*;
pub use crate::println;
use crate::api::clock::*;

pub const NS_NAME_LIMIT : usize = 64;
pub const NS_TID : Tid = 2;

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct NsGetReq<'a> {
    pub name: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct NsSetReq<'a> {
    pub name: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
pub struct NsResp {
    pub tid: Option<Tid>,
}

#[derive(Debug, RecvEnumTrait)]
enum NsRespRecvEnum<'a> {
    NsResp(&'a NsResp),
}

// const DEBUG : bool = true;
const NS_SENDBOX_SZ : usize = NS_NAME_LIMIT + 64;
const NS_RECVBOX_SZ : usize = 64;

fn ns_get_loop(name: &str, cnt: i32) -> Option<Tid> {
    let name_bytes = name.as_bytes();
    let mut ns_sendbox : SendBox<NS_SENDBOX_SZ> = SendBox::default();
    let mut ns_recvbox : RecvBox<NS_RECVBOX_SZ> = RecvBox::default();

    {
        let mut get_req = SendCtx::<NsGetReq>::new(&mut ns_sendbox).unwrap();
        get_req.name = get_req.attach_array(name_bytes.len()).unwrap();
        get_req.name.copy_from_slice(name_bytes);
    }

    let mut i = 1;

    // Runs cnt time, if cnt <= 0, runs infinitely
    loop {
        ker_send(NS_TID, &ns_sendbox, &mut ns_recvbox).unwrap();

        match NsRespRecvEnum::from_recv_bytes(&mut ns_recvbox) {
            Some(NsRespRecvEnum::NsResp( NsResp{tid: Some(tid)} )) => { return Some(*tid); },
            _ => (),
        }

        if cnt > 0 {
            if i >= cnt {
                return None;
            }
            i += 1;
        }

        println!("Waiting for NsGet {name}");

        // wait ticks
        wait_ticks(10);
    }
}

pub fn ns_get(name: &str) -> Option<Tid> {
    ns_get_loop(name, 1)
}

pub fn ns_get_wait(name: &str) -> Tid {
    ns_get_loop(name, 50).unwrap()
}

pub fn ns_set(name: &str) -> Result<Tid, ()> {
    let name_bytes = name.as_bytes();
    let mut ns_sendbox : SendBox<NS_SENDBOX_SZ> = SendBox::default();
    let mut ns_recvbox : RecvBox<NS_RECVBOX_SZ> = RecvBox::default();

    {
        let mut set_req = SendCtx::<NsSetReq>::new(&mut ns_sendbox).unwrap();
        set_req.name = set_req.attach_array(name_bytes.len()).unwrap();
        set_req.name.copy_from_slice(name_bytes);
    } 

    ker_send(NS_TID, &ns_sendbox, &mut ns_recvbox).unwrap();

    match NsRespRecvEnum::from_recv_bytes(&mut ns_recvbox) {
        Some(NsRespRecvEnum::NsResp( NsResp{tid: Some(tid)} )) => Ok(*tid),
        _ => Err(()),
    }
}