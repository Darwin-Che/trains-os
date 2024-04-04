# MsgBox: In-place serializer

# Background

During my course work for CS452 at the University of Waterloo, in a group of two, we developed our own bare metal real time operating system in C. Our OS runs on the a raspberry pi 4b, which controls a toy train set which receives train/switch command and sends sensor signal over RS232.

With the limited time of course work, the compilation and linking for the OS is very simple. Every C file, regardless if it is in kernel space, user space, or even shared, is linked together. So although the correct way to invoke kernel code is via syscall, nothing prevents a user program to call a kernel function directly. This is unsafe.

In addition, this design also doesn’t go in sync with a functionality that I wanted: load user space programs from external binary.

Therefore, after graduation, I started to add more functionality to the OS. Firstly, I added a very simple loader that only process `R_AARCH64_RELATIV` relocation, which is sufficient to support loading a pie binary that does not contain dynamic library. After that, I wanted to write rust code for user space program. Now, straight forwardly, I can compile the rust code into a pie binary and the OS can execute it without even knowing I wrote rust.

The OS has only one IPC mechanism: message passing. Specifically, three syscalls

```c
/*
	One process call ke_send with two buffers.
	msg buffer contains the bytes to send.
	reply buffer will be populated with the bytes sent via ke_reply.
*/
int ke_send(int tid, const char *msg, size_t msglen, char *reply, size_t rplen);
/*
	One process call ke_recv with one buffer.
	msg buffer will be populated with the bytes sent via ke_send.
*/
int ke_recv(int *tid, char *msg, size_t msglen);
/*
	One process can call ke_reply after ke_recv returns.
	reply buffer contains the bytes to send.
*/
int ke_reply(int tid, const char *reply, size_t rplen);
```

| Sender |  | Receiver |
| --- | --- | --- |
|  |  | ke_recv() |
|  |  | (blocked) |
| ke_send() | → | … |
| (blocked) |  | … |
| … | ← | ke_reply() |
| … |  | … |

The OS syscall, out of simplicity, only deal with bytes. But to use this functionality efficiently in Rust, I need a wrapper that can convert information in Rust into bytes efficiently. And that Rust information, likely, is coded in a struct.

Note, we are working under the constraint of 1) `no_std`, 2) no heap allocation.

# Rust Wrapper API

```rust
struct MsgA {
	a: u64,
	x: [u8; 10],
}

struct MsgB {
	b: u64,
	y: AttachedArray<u8>
}

enum RecvEnum<'a> {
	MsgA(&'a mut MsgA),
	MsgB(&'a mut MsgB),
}

fn send_a(tid: Tid) {
	let mut send_buf = [0; 128];
	let mut reply_buf = [0; 128];
	
	// Rent the bytes in send_buf to use msg_a to write the data in correct place
	let mut msg_a = send_prepare::<MsgA>(&mut send_buf);
	msg_a.a = 100;
	for i in 0..10 {
		msg_a.x[i] = i;
	}
	
	ker_send(tid, &send_buf, &mut reply_buf);
	
	// Interpret the bytes in reply_buf,
	// from_recv_bytes() will decide the type of the received message
	// msg_a or msg_b will be renting the bytes in reply_buf, so that
	//   we can read the data in the correct place
	match from_recv_bytes::<RecvEnum>(&mut reply_buf) {
		RecvEnum::MsgA(msg_a) => (),
		RecvEnum::MsgB(msg_b) => (),
	}
}

fn recv_a_or_b() {
	let mut recv_buf = [0; 128];
	
	let tid = ker_recv(&mut recv_buf);
	
	// Interpret the bytes in recv_buf
	match from_recv_bytes::<RecvEnum>(&mut recv_buf) {
		RecvEnum::MsgA(msg_a) => (),
		RecvEnum::MsgB(msg_b) => (),
	}
	
	// Construct the reply
	let mut reply_buf = [0; 128];
	let mut msg_a = send_prepare::<MsgA>(&mut reply_buf);
	msg_a.a = 200;
	for i in 0..10 {
		msg_a.x[i] = i + 10;
	}
	
	ker_reply(tid, &reply_buf);
}
```

I actually didn’t use `MsgB` in the above example. `MsgB` contains an `AttachedArray`. I define `AttachedArray` as a way to send dynamic sized array in a message. The bytes layout in the buffer is like this (with necessary padding between blocks)

| Type ID | struct bytes | AttachedArray 1 | (Other AttachedArrays) |
| --- | --- | --- | --- |

`AttachedArray` will act as a reference to the actual attached bytes. It acts like a Rust reference when reading/writing the message. It encodes the relative offset and size in the whole message when transmitting the message across user programs.

Therefore, the complete API looks like this

```rust
struct MsgA {
	a: u64,
	x: [u8; 10],
}

struct MsgB<'a> {
	b: u64,
	y: AttachedArray<'a, u8>
}

enum RecvEnum<'a> {
	MsgA(&'a mut MsgA),
	MsgB(&'a mut MsgB<'a>),
}

fn send_a(tid: Tid) {
	let mut send_buf = [0; 128];
	let mut reply_buf = [0; 128];
	
	// Rent the bytes in send_buf to use MsgA layout to write the data in correct place
	let mut send_ctx = send_prepare::<MsgA>(&mut send_buf);
	send_ctx.t.a = 100;
	for i in 0..10 {
		send_ctx.t.x[i] = i;
	}
	
	ker_send(tid, &send_buf, &mut reply_buf);
	
	// Interpret the bytes in reply_buf,
	// from_recv_bytes() will decide the type of the received message
	// msg_a or msg_b will be renting the bytes in reply_buf, so that
	//   we can read the data in the correct place
	match from_recv_bytes::<RecvEnum>(&mut reply_buf) {
		Some(RecvEnum::MsgA(msg_a)) => println!(msg_a),
		Some(RecvEnum::MsgB(msg_b)) => println!(msg_b),
		_ => panic!(),
	}
}

fn recv_a_or_b() {
	let mut recv_buf = [0; 128];
	
	let tid = ker_recv(&mut recv_buf);
	
	// Interpret the bytes in recv_buf
	match from_recv_bytes::<RecvEnum>(&mut recv_buf) {
		Some(RecvEnum::MsgA(msg_a)) => println!(msg_a),
		Some(RecvEnum::MsgB(msg_b)) => println!(msg_b),
		_ => panic!(),
	}
	
	// Construct the reply
	let mut reply_buf = [0; 128];
	let mut send_ctx = send_prepare::<MsgB>(&mut reply_buf);
	send_ctx.t.b = 200;
	send_ctx.t.y = send_ctx.attach_array(20);
	for i in 0..20 {
		send_ctx.t.y[i] = i + 10;
	}
	
	ker_reply(tid, &reply_buf);
}
```

The only change is to introduce `send_ctx`. It should be a wrapper on the raw bytes to present the struct and it should be able to allocate attach array from the raw bytes.

# Implementation - Send

On the sender, we need to implement the API `send_ctx`, `attach_array`, and `send_prepare`.

We first define the struct

```rust
struct SendCtx<'a, T> {
		t: &'a mut T,            // The msg struct
		send_buf: &'a mut [u8],  // Trailing bytes in the buffer for attached array allocation
		idx: &'a mut usize,      // Track usage of the original buffer, i.e. the offset of send_buf
}

pub struct AttachedArray<'a, I> {
    idx: u32,
    cnt: u32,
    array: Option<&'a mut [I]>,
}
```

Then `AttachedArray` can be created naturally from `send_buf` in `SendCtx`.
(I will skip the distracting details by using macro helper functions with explanatory names)

```rust
// impl SendCtx<'a, T>
pub fn attach_array<I>(&mut self, cnt: usize) -> Option<AttachedArray<'a, I>> {
    // Calculate padding for alignment of type I
    let aligned_idx = calculate_aligned_idx!(*self.idx, align_of!(I));
    let padding_len = aligned_idx - *self.idx;

    // Check if there is enough space
    if there_is_enough_space!(self.send_buf, aligned_idx, cnt, I) {
        return None;
    }

    // Split the buffer at new idx
    *self.idx += padding_len + cnt * size_of!(I);

	// Need to use mem::take to remove the life time connection between the attached array and self
    let send_buf = mem::take(&mut self.send_buf);
    let (_padding, send_buf_after_padding) = send_buf.split_at_mut(padding_len);
    let (array, send_buf_after_array) = send_buf_after_padding.split_at_mut(cnt * size_of!(I));
    self.send_buf = send_buf_after_array;

    // Return the AttachedArray
    Some(AttachedArray {
        idx: (*self.idx - cnt * size_of!(I)) as u32,
        cnt: cnt as u32,
        array: Some(unsafe{ 
            slice::from_raw_parts_mut(array.as_mut_ptr() as *mut I, cnt)
        }),
    })
}
```

Finally, let’s try to implement `send_prepare`. If we switch perspective, it looks more like a constructor for `SendCtx` instead of a global function.

```rust
// impl SendCtx<'a, T>
pub fn new(buf: &'a mut [u8], buf_used: &'a mut usize) -> Self {
	let msg_id_len = calc_id_len!(T);
	let (buf_id, buf_rest) = buf.split_at_mut(msg_id_len);
	write_msg_id!(buf_id, T);
	let (buf_obj, buf_aa) = buf_rest.split_at_mut(sizeof!(T));
	*buf_used = msg_id_len + sizeof!(T);
	Self {
		t: unsafe { &mut *(buf_obj.as_ptr() as *mut T) },
		send_buf: buf_aa,
		idx: buf_used
	}
}
```

# Implementation - Receive

There is only one function to implement: `from_recv_bytes`. It can be described as this: 1) given all the choices in the enum, choose the appropriate variant based on the first segment of the received bytes, 2) interpret the second part of the bytes as the chosen struct, 3) split the third part of the bytes into chunks according to the attached arrays in the struct from part two, 4) assign the attached arrays to the associated fields in the struct.

Notice that we cannot just take the bytes containing all of the attached arrays and in a loop for each attached array fields we take a slice of the bytes and assign it to the attached array fields. The reason is that it violates the singularity of the mutable reference. The Rust compiler doesn’t know if the program could end up having two attached array referencing overlapped bytes. In summary, we have to first find the split positions in the bytes, split it into segments, then assign each segment to the corresponding attached array.

Another thing to note is that in the first step, the function needs to be aware of all the choices in the enum and in the third step, the function needs to know all the attached array fields in a struct. For these purpose, we can use a Rust `proc_macro`. However, here for the simplicity for illustration, I will write specific functions which will be the result of `proc_macro` expansion. Simply keep in mind that the code sometimes looks weird only because `proc_macro` generates it.

Firstly, let me give the implementation of `from_recv_bytes` generated for `RecvEnum` which calls into another `from_recv_bytes` which is a member function generated for each msg struct.

```rust
// impl RecvEnum<'a>
pub fn from_recv_bytes(buf: &'a mut [u8]) -> Option<Self> {
	match buf {
		[ msg_id_with_padding!(MsgA), data @ .. ] =>
				Some(RecvEnum::MsgA(MsgA::from_recv_bytes(data))),
		[ msg_id_with_padding!(MsgB), data @ .. ] =>
				Some(RecvEnum::MsgB(MsgB::from_recv_bytes(data))),
		_ => None,
	}
}
```

Now, let me give the implementation of the member function `from_recv_bytes` where msg struct is interpreted, and it calls `from_recv_bytes` of `AttachedArray` to split the bytes and assign the reference. Here I will give the example for `MsgB`

```rust
// impl MsgB<'a>
pub fn from_recv_bytes(data: &'a mut [u8]) -> &mut Self {
	let (buf_obj, buf_attached) = data.split_at_mut(size_of!(Self));
	
	let obj = unsafe { &mut *(buf_obj.as_mut_ptr() as *mut Self) };
	
	let mut array_fields = [ &mut obj.y ];
	
	AttachedArray::from_recv_bytes(
		&mut array_fields,
		buf_attached,
		msg_id_with_padding_len!(Self) + size_of!(Self)
	);
	
	obj
}
```

Then there is the implementation of `AttachedArray::from_recv_bytes` which I came up with draining my brain power.

```rust
// impl AttachedArray<'a, I>
pub fn from_recv_bytes(ref_array: &mut [&mut Self], mut buf: &mut [u8], idx_offset: usize) {
	ref_array.sort_unstable_by_key(|aa| aa.idx);
	
	let mut offset = idx_offset;
	
	for aa in ref_array {
		let start = aa.idx - offset;
		// Discard the padding between attached arrays due to alignment
		if start != 0 {
			buf = buf.split_at_mut(start).1;
		}
		
		let split_buf = buf.split_at_mut(aa.cnt * size_of!(I));
		aa.array = Some(unsafe { slice::from_raw_parts_mut(split_buf.0.as_mut_ptr() as *mut I, aa.cnt) });
		
		// Update the buf to the rest of the array
		buf = split_buf.1;
		// Update buf offset with the segment we just cut off
		offset = start + aa.cnt * size_of!(I);
	}
}
```

Now we have most of the code!

# More about Alignment

In the above implementation, we need to take care of alignment in several places. However, we missed a few places:

- the actual byte buffer needs to be properly aligned. Therefore, we should modify the raw buffer we used before by wrapping it in a struct that is 8 bytes aligned.
- in addition, the `msg_id_with_padding!` is also always padded to be a multiple of 8 bytes.

Given the previous two alignment constraint, the actual struct is always on 8 bytes boundary.

# Further Improvements

## More Compact `AttachedArray`

If we change the definition from

```rust
pub struct AttachedArray<'a, I> {
    idx: u32,
    cnt: u32,
    array: Option<&'a mut [I]>,
}
```

to

```rust
#[repr(C)]
pub union AttachedArray<'a, I> {
    idx_cnt: (isize, usize),
    array: Option<&'a mut [I]>,
}
```

We get to save 8 bytes per attached array field; however, we need to modify the functions a little bit.

- Receive reads `idx_cnt` and overwrites it with `array`.
- Send overwrites array with `idx_cnt` after `array` is filled out and before actually sending the buffer. This can be implemented by using the `Drop` trait for `SendCtx`.

## More Accessible `SendCtx`

Currently, we need to use `send_ctx.t` to reference the msg struct to send.

We can implement the `Deref` and `DerefMut` trait for `send_ctx` so that we can directly use it

```rust
	let mut msg_b = SendCtx::<MsgB>::new(&mut reply_buf);
	msg_b.b = 200;
	msg_b.y = msg_b.attach_array(20);
	for i in 0..20 {
		msg_b.y.array[i] = i + 10;
	}
```

Furthermore, if we implement `Deref` and `DerefMut` on `AttachArray`, we will be able to do this

```rust
msg_b.y.array[i] = i + 10;
// =>
msg_b.y[i] = i + 10;
```

Now the syntax is much more concise.