use crate::println;
use heapless::Vec;
pub use core::ffi::c_char;
use core::slice;
use core::str;

pub struct EntryArgs {
    pairs: Vec<(&'static str, &'static str), 32>,
}

impl EntryArgs {
    pub fn new(ptr: *const c_char, len: usize) -> Self {
        let mut e = Self{ pairs: Vec::new() };
        let mut bytes = unsafe { slice::from_raw_parts(ptr, len) };

        for [key, val] in bytes.split(|b| *b == 0).array_chunks() {
            e.pairs.push((
                str::from_utf8(key).unwrap(), 
                str::from_utf8(val).unwrap()
            )).unwrap();
        }

        e
    }

    pub fn print(&self) {
        println!("{:?}", self.pairs);
    }

    pub fn get(&self, key: &str) -> Option<&'static str> {
        for (k, v) in &self.pairs {
            if *k == key {
                return Some(v);
            }
        }
        return None;
    }
}