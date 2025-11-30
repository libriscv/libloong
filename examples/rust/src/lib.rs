mod kernel;

use std::{fmt, time::{Duration, Instant}};

use size::{Base, Size};

pub use crate::kernel::*;

struct Timings {
    min: Duration,
    max: Duration,
    total: Duration,
}

impl Default for Timings {
    fn default() -> Self {
        Self {
            min: Duration::MAX,
            max: Duration::ZERO,
            total: Duration::ZERO,
        }
    }
}

impl Timings {
    fn push(&mut self, d: Duration) {
        self.min = Duration::min(self.min, d);
        self.max = Duration::max(self.max, d);
        self.total += d;
    }
}

pub struct Rate {
    min: Duration,
    max: Duration,
    avg: Duration,
    size: Size
}

impl fmt::Display for Rate {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{1:.0$}/s | time min {2:.0$?} avg {3:.0$?} max {4:.0$?}",
            f.precision().unwrap_or(1),
            (self.size / self.min.as_secs_f64()).format().with_base(Base::Base10),
            self.min, self.avg, self.max)
    }
}

pub fn bench(mut kernel: Box<dyn Kernel>, runs: usize) -> Rate {
    let mut timings = Timings::default();

    kernel.init();
    kernel.run();

    for _ in 1..runs {
        let start = Instant::now();
        kernel.run();
        let elapsed = start.elapsed();
        timings.push(elapsed);
    }

    Rate {
        min: timings.min,
        max: timings.max,
        avg: timings.total / (runs - 1) as u32,
        size: kernel.size(),
    }
}
