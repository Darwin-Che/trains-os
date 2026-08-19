The original kernel is developed as the course project for CS452 by Zhaocheng Che and Skasham Bajaj.

The microkernel from CS452 is preserved, the train control program is stripped to leave more fun for future students.

## Extension

0. common code & external lib: `lib/`
1. kernel: `kernel/`
2. loader for static PIE: `loader/src`
3. user space program in C: `user/`
4. user space program in Rust: `pie-rust`

## Example Rust Snippet

```
### Types

#[derive(Debug, RecvEnumTrait)]
#[allow(dead_code)]
enum RecvEnum<'a> {
    L2capPacket(&'a mut L2capPacket<'a>),
    ...
}

pub struct L2capPacket<'a> {
    pub acl_header: [u8; 2],
    pub chnl_id: u16,
    pub data: AttachedArray<'a, u8>,
}

### Function

let mut send_box: SendBox = SendBox::default();
let mut recv_box: RecvBox = RecvBox::default();

loop {
    let sender_tid = ker_recv(&mut recv_box);
    match RecvEnum::from_recv_bytes(&mut recv_box) {
        Some(RecvEnum::L2capPacket(packet)) => {
            SendCtx::<HciReply>::new(&mut send_box).unwrap();
            ker_reply(sender_tid, &send_box).unwrap();

            match packet.data[0] {
                ...
            }
        }
    }
}
```
