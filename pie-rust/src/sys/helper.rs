// impl<T, const N: usize> Index<usize> for Deque<T, N> {
//     type Output = Option<T>;

//     fn index(&self, idx: usize) -> &Self::Output {
//         let (s1, s2) = self.as_slices();
//         if idx < s1.len() {
//             Some(s1[idx])
//         } else if idx < s1.len() + s2.len() {
//             Some(s2[idx - s1.len()])
//         } else {
//             None
//         }
//     }
// }

pub fn read_u16(bytes: &[u8]) -> u16 {
    if bytes.len() < 1 {
        return 0;
    } else if bytes.len() < 2 {
        return bytes[0] as u16;
    } else {
        return (bytes[1] as u16) << 8 | (bytes[0] as u16);
    }
}