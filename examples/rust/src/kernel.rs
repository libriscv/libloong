use std::fmt;

use num_traits::{cast, Num, NumCast};
use size::Size;

pub trait Kernel: fmt::Display {
    fn size(&self) -> Size;

    fn init(&mut self);

    fn run(&mut self);
}

/// Filling kernel
///
/// ```
/// a(i) = x
/// ```
///
/// Bytes/iter read = sizeof(T)
///
/// Bytes/iter write = sizeof(T)
///
/// FLOPS/iter = 0
pub struct KernelFill<T> {
    a: Vec<T>,
    size: usize,
}

impl<T> KernelFill<T> {
    pub fn new(size: usize) -> Self {
        Self {
            a: Vec::new(),
            size,
        }
    }
}

impl<T> Kernel for KernelFill<T>
where
    T: Num + NumCast + Copy + Send
{
    fn size(&self) -> Size {
        Size::from_bytes(size_of::<T>() * self.size)
    }

    fn init(&mut self) {
        self.a = vec![cast(0).unwrap(); self.size];
    }

    fn run(&mut self) {
        self.a.iter_mut().for_each(|a| *a = cast(3).unwrap());
    }
}

impl<T> fmt::Display for KernelFill<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Fill {}", Size::from_bytes(size_of::<T>() * self.size))
    }
}

/// Copying kernel
///
/// ```
/// a(i) = b(i)
/// ```
///
/// Bytes/iter read = 2 * sizeof(T)
///
/// Bytes/iter write = sizeof(T)
///
/// FLOPS/iter = 1
pub struct KernelCopy<T> {
    a: Vec<T>,
    b: Vec<T>,
    size: usize,
    offset: usize,
}

impl<T> KernelCopy<T> {
    pub fn new(size: usize, offset: usize) -> Self {
        Self {
            a: Vec::new(),
            b: Vec::new(),
            size, offset,
        }
    }
}

impl<T> Kernel for KernelCopy<T>
where
    T: Num + NumCast + Copy + Send + Sync
{
    fn size(&self) -> Size {
        Size::from_bytes(2 * size_of::<T>() * self.size)
    }

    fn init(&mut self) {
        self.a = vec![cast(0).unwrap(); self.size + self.offset];
        self.b = vec![cast(1).unwrap(); self.size];
    }

    fn run(&mut self) {
        self.a.iter_mut().take(self.size).zip(self.b.iter())
            .for_each(|(a, &b)| *a = b);
    }
}

impl<T> fmt::Display for KernelCopy<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Copy {}", Size::from_bytes(2 * size_of::<T>() * self.size))
    }
}

/// Scaling kernel
///
/// ```
/// a(i) = b(i) * x
/// ```
///
/// Bytes/iter read = 2 * sizeof(T)
///
/// Bytes/iter write = sizeof(T)
///
/// FLOPS/iter = 1
pub struct KernelScale<T> {
    a: Vec<T>,
    b: Vec<T>,
    size: usize,
    offset: usize,
}

impl<T> KernelScale<T> {
    pub fn new(size: usize, offset: usize) -> Self {
        Self {
            a: Vec::new(),
            b: Vec::new(),
            size, offset,
        }
    }
}

impl<T> Kernel for KernelScale<T>
where
    T: Num + NumCast + Copy + Send + Sync
{
    fn size(&self) -> Size {
        Size::from_bytes(2 * size_of::<T>() * self.size)
    }

    fn init(&mut self) {
        self.a = vec![cast(0).unwrap(); self.size + self.offset];
        self.b = vec![cast(1).unwrap(); self.size];
    }

    fn run(&mut self) {
        self.a.iter_mut().take(self.size).zip(self.b.iter())
            .for_each(|(a, &b)| *a = b * cast(3).unwrap());
    }
}

impl<T> fmt::Display for KernelScale<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Scale {}", Size::from_bytes(2 * size_of::<T>() * self.size))
    }
}

/// Adding kernel
///
/// ```
/// a(i) = b(i) + c(i)
/// ```
///
/// Bytes/iter read = 3 * sizeof(T)
///
/// Bytes/iter write = sizeof(T)
///
/// FLOPS/iter = 1
pub struct KernelAdd<T> {
    a: Vec<T>,
    b: Vec<T>,
    c: Vec<T>,
    size: usize,
    offset: usize,
}

impl<T> KernelAdd<T> {
    pub fn new(size: usize, offset: usize) -> Self {
        Self {
            a: Vec::new(),
            b: Vec::new(),
            c: Vec::new(),
            size, offset,
        }
    }
}

impl<T> Kernel for KernelAdd<T>
where
    T: Num + NumCast + Copy + Send + Sync
{
    fn size(&self) -> Size {
        Size::from_bytes(3 * size_of::<T>() * self.size)
    }

    fn init(&mut self) {
        self.a = vec![cast(0).unwrap(); self.size + self.offset];
        self.b = vec![cast(1).unwrap(); self.size + self.offset];
        self.c = vec![cast(2).unwrap(); self.size];
    }

    fn run(&mut self) {
        self.a.iter_mut().take(self.size).zip(self.b.iter().zip(self.c.iter()))
            .for_each(|(a, (&b, &c))| *a = b + c);
    }
}

impl<T> fmt::Display for KernelAdd<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Add {}", Size::from_bytes(3 * size_of::<T>() * self.size))
    }
}

/// Triad kernel
///
/// ```
/// a(i) = b(i) + c(i) * x
/// ```
///
/// Bytes/iter read = 3 * sizeof(T)
///
/// Bytes/iter write = sizeof(T)
///
/// FLOPS/iter = 2
pub struct KernelTriad<T> {
    a: Vec<T>,
    b: Vec<T>,
    c: Vec<T>,
    size: usize,
    offset: usize,
}

impl<T> KernelTriad<T> {
    pub fn new(size: usize, offset: usize) -> Self {
        Self {
            a: Vec::new(),
            b: Vec::new(),
            c: Vec::new(),
            size, offset,
        }
    }
}

impl<T> Kernel for KernelTriad<T>
where
    T: Num + NumCast + Copy + Send + Sync
{
    fn size(&self) -> Size {
        Size::from_bytes(3 * size_of::<T>() * self.size)
    }

    fn init(&mut self) {
        self.a = vec![cast(0).unwrap(); self.size + self.offset];
        self.b = vec![cast(1).unwrap(); self.size + self.offset];
        self.c = vec![cast(2).unwrap(); self.size];
    }

    fn run(&mut self) {
        self.a.iter_mut().take(self.size).zip(self.b.iter().zip(self.c.iter()))
            .for_each(|(a, (&b, &c))| *a = b + c * cast(3).unwrap());
    }
}

impl<T> fmt::Display for KernelTriad<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Triad {}", Size::from_bytes(3 * size_of::<T>() * self.size))
    }
}

/// Summing kernel
///
/// ```
/// x += a(i)
/// ```
///
/// Bytes/iter read = sizeof(T)
///
/// Bytes/iter write = 0
///
/// FLOPS/iter = 1
pub struct KernelSum<T> {
    a: Vec<T>,
    size: usize,
}

impl<T> KernelSum<T> {
    pub fn new(size: usize) -> Self {
        Self {
            a: Vec::new(),
            size,
        }
    }
}

impl<T> Kernel for KernelSum<T>
where
    T: Num + NumCast + Copy + Send + Sync,
{
    fn size(&self) -> Size {
        Size::from_bytes(size_of::<T>() * self.size)
    }

    fn init(&mut self) {
        self.a = vec![cast(1).unwrap(); self.size];
    }

    fn run(&mut self) {
        self.a.iter().fold(T::zero(), |acc, &x| acc + x);
    }
}

impl<T> fmt::Display for KernelSum<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Sum {}", Size::from_bytes(size_of::<T>() * self.size))
    }
}
