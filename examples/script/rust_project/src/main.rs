// Include auto-generated API bindings
#![allow(unused)]
#![allow(dead_code)]
#![allow(non_camel_case_types)]

#[path = "../libloong_api.rs"]
mod libloong_api;
use libloong_api::*;

use std::ffi::c_int;

// Example guest functions that can be called from host

#[no_mangle]
pub extern "C" fn compute(a: c_int, b: c_int) -> c_int {
    unsafe {
        host_print(a);
        host_print(b);
        let sum = host_add(a, b);
        host_print(sum);
        sum
    }
}

#[no_mangle]
pub extern "C" fn calculate_area(radius: f32) -> f32 {
    let r2 = radius * radius;
    3.14159 * r2
}

#[no_mangle]
pub extern "C" fn test_counter() -> c_int {
    unsafe {
        let initial = get_counter();
        increment_counter();
        increment_counter();
        increment_counter();
        let after = get_counter();

        reset_counter();
        let _reset_val = get_counter();

        after - initial // Should be 3
    }
}

#[no_mangle]
pub extern "C" fn factorial(n: c_int) -> c_int {
    if n <= 1 {
        1
    } else {
        n * factorial(n - 1)
    }
}

#[no_mangle]
pub extern "C" fn test_string_operations() -> c_int {
    unsafe {
        // Create a Rust String and pass it to the host
        let test_str = String::from("Hello from Rust!");

        // Pass the reference directly - the API now accepts &String
        let len = rust_string_length(&test_str);

        // Log it too
        rust_log_message(&test_str);

        len
    }
}

#[no_mangle]
pub extern "C" fn test_vector_operations() -> c_int {
	let numbers: Vec<c_int> = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    unsafe {
        rust_print_vector_sum(&numbers);
    }
	numbers.len() as c_int
}

fn main() {
    unsafe {
        fast_exit(0);
    }
}
