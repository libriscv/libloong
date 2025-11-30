use clap::Parser;
use num_traits::{Num, NumCast};
use stream::*;

/// STREAM benchmark
///
/// The STREAM benchmark is a simple synthetic benchmark program that measures sustainable
/// memory bandwidth and the corresponding computation rate for simple vector kernels.
///
/// Documentation for STREAM is available on the web at: http://www.cs.virginia.edu/stream/ref.html
///
/// 1. McCalpin, John D. (1995). "Memory bandwidth and machine balance in current high performance computers."
/// IEEE computer society technical committee on computer architecture (TCCA) newsletter, vol. 2, 19-25.
#[derive(Debug, Parser)]
#[command(version, about("STREAM benchmark"))]
struct Args {
    /// Array size (elements)
    #[arg(short('s'), long, default_value_t = 10_000_000)]
    size: usize,

    /// Array offset (elements)
    #[arg(short('o'), long, default_value_t = 0)]
    offset: usize,

    /// Array element type
    #[arg(short('t'), long)]
    #[clap(value_enum, default_value_t = ArrayType::Double)]
    r#type: ArrayType,

    /// Run a specific kernel
    ///
    /// If unspecified, runs all available kernels
    #[arg(short('k'), long)]
    #[clap(value_enum)]
    kernel: Option<KernelType>,

    /// Number of measurement iterations, including one warmup run [min: 2]
    #[arg(short('r'), long, default_value_t = 20, value_parser = validate_runs)]
    runs: usize,
}

fn validate_runs(runs: &str) -> Result<usize, String> {
    runs.parse::<usize>()
        .map_err(|e| e.to_string())
        .and_then(|runs| {
            if runs >= 2 {
                Ok(runs)
            } else {
                Err("minimum number of runs is 2".into())
            }
        })
}

#[derive(Copy, Clone, Debug)]
#[derive(clap::ValueEnum)]
enum ArrayType {
    Int,
    Long,
    Float,
    Double,
}

#[derive(Copy, Clone, Debug)]
#[derive(clap::ValueEnum)]
enum KernelType {
    Fill,
    Copy,
    Scale,
    Add,
    Triad,
    Sum,
}

impl KernelType {
    fn build<T>(self, size: usize, offset: usize) -> Box<dyn Kernel>
    where
        T: Num + NumCast + Copy + Send + Sync + 'static
    {
        use KernelType::*;
        match self {
            Fill  => Box::new(KernelFill::<T>::new(size)),
            Copy  => Box::new(KernelCopy::<T>::new(size, offset)),
            Scale => Box::new(KernelScale::<T>::new(size, offset)),
            Add   => Box::new(KernelAdd::<T>::new(size, offset)),
            Triad => Box::new(KernelTriad::<T>::new(size, offset)),
            Sum   => Box::new(KernelSum::<T>::new(size)),
        }
    }
}

fn main() {
    let Args {
        size,
        offset,
        r#type,
        runs,
        kernel,
    } = Args::parse();

    let kernels: Vec<Box<dyn Kernel>> = {
        use ArrayType::*;
        if let Some(kernel) = kernel {
            vec![match r#type {
                Int => kernel.build::<i32>(size, offset),
                Long => kernel.build::<i64>(size, offset),
                Float => kernel.build::<f32>(size, offset),
                Double => kernel.build::<f64>(size, offset),
            }]
        } else {
            fn build<T>(size: usize, offset: usize) -> Vec<Box<dyn Kernel>>
            where
                T: Num + NumCast + Copy + Send + Sync + 'static
            {
                vec![
                    Box::new(KernelFill::<T>::new(size)),
                    Box::new(KernelCopy::<T>::new(size, offset)),
                    Box::new(KernelScale::<T>::new(size, offset)),
                    Box::new(KernelAdd::<T>::new(size, offset)),
                    Box::new(KernelTriad::<T>::new(size, offset)),
                    Box::new(KernelSum::<T>::new(size)),
                ]
            }

            match r#type {
                Int => build::<i32>(size, offset),
                Long => build::<i64>(size, offset),
                Float => build::<f32>(size, offset),
                Double => build::<f64>(size, offset),
            }
        }
    };

    let names: Vec<String> = kernels.iter().map(|k| k.to_string()).collect();
    let width = names.iter().map(|s| s.len()).max().unwrap();

    for (kernel, name) in kernels.into_iter().zip(names.iter()) {
        let rate = bench(kernel, runs);
        println!("{:width$} rate {}", name, rate);
    }
}
