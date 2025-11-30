// Simple Rust program using libc and a regular main function
// This is compiled as a normal statically-linked executable

fn fibonacci(n: u64) -> u64 {
    if n <= 1 {
        n
    } else {
        fibonacci(n - 1) + fibonacci(n - 2)
    }
}

fn main() {
    println!("Hello from Rust on LoongArch!");

    // Demonstrate basic computation
    let a = 42u64;
    let b = 13u64;
    let sum = a + b;

    println!("Addition: {} + {} = {}", a, b, sum);

    // Demonstrate more complex computation
    let fib_n = 20;
    let result = fibonacci(fib_n);
    println!("Fibonacci({}) = {}", fib_n, result);

    // Demonstrate string operations
    let message = format!("Rust is running on LoongArch! Result: {}", result);
    println!("{}", message);

    // Demonstrate vector operations
    let numbers: Vec<i32> = (1..=10).collect();
    let sum: i32 = numbers.iter().sum();
    println!("Sum of 1..10 = {}", sum);
}
