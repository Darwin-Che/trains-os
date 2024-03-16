use core::marker::PhantomData;
use core::{mem, slice};
use msgbox_macro::{RecvEnumTrait, MsgTrait};
use super::types::*;
use core::ffi::{c_char, c_int};

/* EXAMPLE

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
struct UartPutsReq<'a> {
    // Please use 'a for the lifetime if needed, MsgTrait relies on it
    blocking: bool,
    msg: AttachedArray<'a, u8>,
}

#[repr(C)]
#[derive(Debug, Default, MsgTrait)]
struct UartGetsReq {
    blocking: bool,
    msg_len: u32,
}

#[derive(Debug, RecvEnumTrait)]
enum RecvEnum<'a> {
    UartPutsReq(&'a UartPutsReq<'a>),
    UartGetsReq(&'a UartGetsReq),
}

fn main() {
    let mut send_buf: SendBox = SendBox::default();
    {
        let (mut send_box, mut uart_puts_req) = SendCtr::<UartPutsReq>::new(&mut send_buf.send_buf).unwrap();
        uart_puts_req.msg = send_box.attach_array(5).unwrap();
        uart_puts_req.msg.array.copy_from_slice("mPuts".as_bytes());
    }

    println!("send_buf\n{:?}", send_buf.send_buf);

    let mut recv_box: RecvBox = RecvBox::new();
    recv_box.recv_test(&send_buf.send_buf);
    let d = RecvEnum::from_recv_bytes(&mut recv_box.recv_buf);

    println!("{:?}", d);
}

*/

/* MsgTrait */

pub trait MsgTrait<'a> : Default {
    fn type_name() -> &'static str {
        &"MsgTrait"
    }
    fn resolve_attached_array_ref(&'a mut self, _buf: &'a mut [u8]) {}
    fn resolve_after_recv(&'a mut self, buf: &'a mut [u8]) {
        self.resolve_attached_array_ref(buf);
    }
}

const DEFAULT_BOX_SIZE: usize = 1024;

/* Recv */

pub struct RecvBox<const N: usize = DEFAULT_BOX_SIZE> {
    pub recv_buf: [u8; N],
}

impl<const N: usize> RecvBox<N> {
    pub fn new() -> Self {
        RecvBox {
            recv_buf: [0u8; N]
        }
    }

    pub fn recv_test(&mut self, test_recv_buf: &[u8]) {
        self.recv_buf[..test_recv_buf.len()].copy_from_slice(&test_recv_buf);
    }
}

pub trait RecvEnumTrait<'a> {
    fn from_recv_bytes<const N: usize>(recv_box: &'a mut RecvBox<N>) -> Option<Self> where Self: Sized;
}

/* Send */

#[repr(align(64))]
pub struct SendBox<const N: usize = DEFAULT_BOX_SIZE> {
    len: usize,
    send_buf: [u8; N],
}

impl<const N: usize> SendBox<N> {
    pub fn as_slice(&mut self) -> &mut [u8] {
        &mut self.send_buf[..self.len]
    }
}

impl<const N: usize> Default for SendBox<N> {
    fn default() -> SendBox<N> {
        SendBox {
            len: 0,
            send_buf: [0u8; N],
        }
    }
}

pub struct SendCtr<'a, T> {
    send_buf: &'a mut [u8],
    idx: &'a mut usize,
    phantom: PhantomData<T>,
}

impl<'a, T> SendCtr<'a, T> {
    pub fn new<const N: usize>(send_box: &'a mut SendBox<N>) -> Option<(Self, &'a mut T)> where T: MsgTrait<'a> {
        let send_buf = &mut send_box.send_buf;
        let send_type = T::type_name();
        // Calculate the minimum length we need
        // type_str + padding + T
        let send_type_len = send_type.len();
        let padding_len = ((send_type_len + 8) & !7) - send_type_len;
        let struct_len = mem::size_of::<T>();
        let total_len = send_type_len + padding_len + struct_len;
        send_box.len = total_len;
        // Check if send_buf is large enough
        if send_buf.len() < total_len {
            return None;
        }
        // Split send_buf for ownership
        let (send_buf, send_buf_rest) = send_buf.split_at_mut(total_len);
        // Write send_type
        // println!("send_type = {:?}", send_type);
        send_buf[..send_type_len].copy_from_slice(send_type.as_bytes());
        // Write padding
        for i in send_type_len..(send_type_len+padding_len) {
            send_buf[i] = 0u8;
        }
        // Extract &'a mut T
        let t = unsafe { &mut *(send_buf[send_type_len+padding_len..total_len].as_ptr() as *mut T) };
        *t = T::default();
        let s = Self {
            send_buf: send_buf_rest,
            idx: &mut send_box.len,
            phantom: PhantomData::default(),
        };
        Some((s, t))
    }

    pub fn attach_array<I>(&'a mut self, cnt: u32) -> Option<AttachedArray<'a, I>> {
        let cnt_usize = cnt as usize;

        let align = mem::align_of::<I>();
        let tsize = mem::size_of::<I>();

        // Calculate padding for alignment of type I
        let aligned_idx = (*self.idx + align - 1) & !(align - 1);
        let padding_len = aligned_idx - *self.idx;

        // Check if there is enough space
        if self.send_buf.len() < padding_len + cnt_usize * tsize {
            return None;
        }

        // Split the buffer at new idx
        *self.idx += padding_len + cnt_usize * tsize;
        let (_padding, send_buf_after_padding) = self.send_buf.split_at_mut(padding_len);
        let (array, send_buf_after_array) = send_buf_after_padding.split_at_mut(cnt_usize * tsize);
        self.send_buf = send_buf_after_array;

        // Return the AttachedArray
        Some(AttachedArray {
            idx: (*self.idx - cnt_usize * tsize) as u32,
            cnt: cnt,
            array: unsafe{ 
                slice::from_raw_parts_mut(array.as_mut_ptr() as *mut I, cnt_usize)
            },
        })
    }
}

/* AttachedArray */

#[repr(C)]
#[derive(Debug, Default)]
pub struct AttachedArray<'a, T> {
    // {idx, cnt} encodes the relative position to the byte stream
    // {array} encodes the absolution address of the slice
    /*
        When we create the message at the sender, the idx and cnt and array are filled.
        When the receiver gets the message, it writes the array to the correct address in the buffer.

        Actually we can use a union because we are never going to use either field at any time,
        but I decide to simplify things a bit and waste a constant overhead.
        Unfortunately, now an attached array will cost an overhead of 16 bytes.
     */
    idx: u32,
    cnt: u32,
    pub array: &'a mut [T],
}

impl<'a, T> AttachedArray<'a, T> {
    pub fn calc_array(&mut self, buf: &'a mut [u8]) {
        self.array = unsafe {
            slice::from_raw_parts_mut(
                buf[self.idx as usize..(self.idx as usize + self.cnt as usize * mem::size_of::<T>())].as_mut_ptr() as *mut T,
                self.cnt as usize
            )
        };
    }
}

/* Syscall Wrapper */

#[link(name = "syscall")]
extern "C" {
    fn ke_recv(tid: *mut c_int, recv_buf: *mut c_char, buf_len: usize);
    fn ke_reply(tid: c_int, reply_buf: *const c_char, buf_len: usize);
    fn ke_send(tid: c_int, send_buf: *const c_char, send_buf_len: usize, reply_buf: *mut c_char, reply_buf_len: usize);
}

pub fn ker_recv<const R: usize>(tid: &mut Tid, recv_box: &mut RecvBox<R>) {
    unsafe {
        ke_recv(
            tid as *mut c_int,
            recv_box.recv_buf.as_mut_ptr() as *mut c_char,
            R
        )
    }
}

pub fn ker_reply<const S: usize>(tid: Tid, send_box: &SendBox<S>) {
    unsafe {
        ke_reply(
            tid as c_int,
            send_box.send_buf.as_ptr() as *const c_char,
            send_box.len
        )
    }
}

pub fn ker_send<const S: usize, const R: usize>(tid: Tid, send_box: &SendBox<S>, recv_box: &mut RecvBox<R>) {
    unsafe {
        ke_send(
            tid as c_int,
            send_box.send_buf.as_ptr() as *const c_char,
            send_box.len,
            recv_box.recv_buf.as_mut_ptr() as *mut c_char,
            R
        )
    }
}
