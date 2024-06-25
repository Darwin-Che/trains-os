use heapless::Deque;
use core::ops::Index;

impl<T, const N: usize> Index<usize> for Deque<T, N> {
    type Output = Option<T>;

    fn index(&self, idx: usize) -> &Self::Output {
        let (s1, s2) = self.as_slices();
        if idx < s1.len() {
            Some(s1[idx])
        } else if idx < s1.len() + s2.len() {
            Some(s2[idx - s1.len()])
        } else {
            None
        }
    }
}