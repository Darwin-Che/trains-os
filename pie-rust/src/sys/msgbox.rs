use core::{mem, slice};
use super::types::*;
use core::ffi::{c_char, c_int};
use core::ops::{Deref, DerefMut};
// use crate::println;

/* MsgTrait */

pub trait MsgTrait<'a> : Default {
    fn type_name() -> &'static str {
        &"MsgTrait"
    }

    fn from_recv_bytes(buf: &'a mut [u8]) -> &'a mut Self {
        unsafe { &mut *(buf.as_mut_ptr() as *mut Self) }
    }
}

const DEFAULT_BOX_SIZE: usize = 1024;

/* Recv */

#[repr(align(64))]
pub struct RecvBox<const N: usize = DEFAULT_BOX_SIZE> {
    pub recv_buf: [u8; N],
}

impl<const N: usize> Default for RecvBox<N> {
    fn default() -> Self {
        RecvBox {
            recv_buf: [0u8; N]
        }
    }
}

impl<const N: usize> RecvBox<N> {
    pub fn new() -> Self {
        Self::default()
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

impl<const N: usize> Default for SendBox<N> {
    fn default() -> SendBox<N> {
        SendBox {
            len: 0,
            send_buf: [0u8; N],
        }
    }
}

impl<const N: usize> SendBox<N> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn as_slice(&mut self) -> &mut [u8] {
        &mut self.send_buf[..self.len]
    }
}

pub struct SendCtx<'a, T> {
    t: &'a mut T,
    send_buf: &'a mut [u8],
    idx: &'a mut usize,
}

impl<'a, T> Deref for SendCtx<'a, T> {
    type Target = &'a mut T;

    fn deref(&self) -> &Self::Target {
        &self.t
    }
}

impl<'a, T> DerefMut for SendCtx<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.t
    }
}

impl<'a, T> SendCtx<'a, T> {
    pub fn new<const N: usize>(send_box: &'a mut SendBox<N>) -> Option<Self> where T: MsgTrait<'a> {
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
        let (buf_type, buf_after_type) = send_buf.split_at_mut(send_type_len + padding_len);
        let (buf_struct, buf_after_struct) = buf_after_type.split_at_mut(struct_len);
        // Write send_type
        // println!("send_type = {:?}", send_type);
        buf_type[..send_type_len].copy_from_slice(send_type.as_bytes());
        // Write padding
        buf_type[send_type_len..].fill(0u8);
        // Extract &'a mut T
        let t = unsafe { &mut *(buf_struct.as_ptr() as *mut T) };
        *t = T::default();
        let s = Self {
            t: t,
            send_buf: buf_after_struct,
            idx: &mut send_box.len,
        };
        Some(s)
    }

    pub fn attach_array<I>(&mut self, cnt: usize) -> Option<AttachedArray<'a, I>> {
        let align = mem::align_of::<I>();
        let tsize = mem::size_of::<I>();

        // Calculate padding for alignment of type I
        let aligned_idx = (*self.idx + align - 1) & !(align - 1);
        let padding_len = aligned_idx - *self.idx;

        // Check if there is enough space
        if self.send_buf.len() < padding_len + cnt * tsize {
            return None;
        }

        // Split the buffer at new idx
        *self.idx += padding_len + cnt * tsize;

        // Need to use mem::take to remove the life time connection between the attached array and self
        let send_buf = mem::take(&mut self.send_buf);
        let (_padding, send_buf_after_padding) = send_buf.split_at_mut(padding_len);
        let (array, send_buf_after_array) = send_buf_after_padding.split_at_mut(cnt * tsize);
        self.send_buf = send_buf_after_array;

        // Return the AttachedArray
        Some(AttachedArray {
            idx: (*self.idx - cnt * tsize) as u32,
            cnt: cnt as u32,
            array: Some(unsafe{ 
                slice::from_raw_parts_mut(array.as_mut_ptr() as *mut I, cnt)
            }),
        })
    }
}

/* AttachedArray */

#[repr(C)]
#[derive(Debug, Default)]
pub struct AttachedArray<'a, I> {
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
    array: Option<&'a mut [I]>,
}

impl<'a, I> AttachedArray<'a, I> {
    pub fn from_recv_bytes(ref_array: &mut [&mut Self], mut buf: &mut [u8], idx_offset: usize) {
        // println!("AttachedArray::from_recv_bytes {idx_offset} START {:p}", buf);
        // println!("BUF {:?}", &buf[0..32]);

        // firstly, sort attached array by idx
        ref_array.sort_unstable_by_key(|aa| aa.idx);

        let mut offset = idx_offset as u32;

        // For each attached array
        for aa in ref_array {
            let start = aa.idx - offset;
            if start != 0 {
                buf = buf.split_at_mut(start as usize).1;
            }

            // Split the buf for the segment containing this array
            let split_buf = buf.split_at_mut(aa.cnt as usize * mem::size_of::<I>());
            aa.array = Some(unsafe { slice::from_raw_parts_mut(split_buf.0.as_mut_ptr() as *mut I, aa.cnt as usize) });

            // Update the buf to the rest of the array
            buf = split_buf.1;
            // Update buf offset with the segment we just cut off
            offset = start + aa.cnt * mem::size_of::<I>() as u32;
        }
    }
}

impl<'a, I> Deref for AttachedArray<'a, I> {
    type Target = [I];

    fn deref(&self) -> &Self::Target {
        self.array.as_ref().unwrap()
    }
}

impl<'a, I> DerefMut for AttachedArray<'a, I> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.array.as_mut().unwrap()
    }
}

impl<'a> Write for AttachedArray<'a, u8> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.array.as_mut().unwrap()
    }

    fn flush(&mut self) -> Result<()> {
        Ok(())
    }
}

/* Syscall Wrapper */

#[link(name = "syscall")]
extern "C" {
    fn ke_recv(tid: *mut c_int, recv_buf: *mut c_char, buf_len: usize) -> c_int;
    fn ke_reply(tid: c_int, reply_buf: *const c_char, buf_len: usize) -> c_int;
    fn ke_send(tid: c_int, send_buf: *const c_char, send_buf_len: usize, reply_buf: *mut c_char, reply_buf_len: usize) -> c_int;
}

pub fn ker_recv<const R: usize>(recv_box: &mut RecvBox<R>) -> Tid {
    let mut tid: Tid = 0;
    unsafe {
        ke_recv(
            &mut tid as *mut c_int,
            recv_box.recv_buf.as_mut_ptr() as *mut c_char,
            R
        );
    }
    tid
}

pub fn ker_reply<const S: usize>(tid: Tid, send_box: &SendBox<S>) -> Result<(), i32> {
    let ret = unsafe {
        ke_reply(
            tid as c_int,
            send_box.send_buf.as_ptr() as *const c_char,
            send_box.len
        )
    };
    if ret < 0 {
        Err(ret as i32)
    } else {
        Ok(())
    }
}

pub fn ker_send<const S: usize, const R: usize>(tid: Tid, send_box: &SendBox<S>, recv_box: &mut RecvBox<R>) -> Result<(), i32> {
    let ret = unsafe {
        ke_send(
            tid as c_int,
            send_box.send_buf.as_ptr() as *const c_char,
            send_box.len,
            recv_box.recv_buf.as_mut_ptr() as *mut c_char,
            R
        )
    };
    if ret < 0 {
        Err(ret as i32)
    } else {
        Ok(())
    }
}
