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

fn main() {
    unsafe {
        fast_exit(0);
    }
}
