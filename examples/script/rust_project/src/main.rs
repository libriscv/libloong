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
    host_print(a);
    host_print(b);
    let sum = host_add(a, b);
    host_print(sum);
    sum
}

#[no_mangle]
pub extern "C" fn calculate_area(radius: f32) -> f32 {
    let r2 = radius * radius;
    3.14159 * r2
}

#[no_mangle]
pub extern "C" fn test_counter() -> c_int {
    let initial = get_counter();
    increment_counter();
    increment_counter();
    increment_counter();
    let after = get_counter();
    println!("  [GUEST] Counter: initial = {}, after = {}", initial, after);

    reset_counter();
    let _reset_val = get_counter();

    after - initial // Should be 3
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
pub extern "C" fn greet(name: &String) {
    let greeting = format!("Hello, {}!", name);
    log_message(&greeting);
}

#[no_mangle]
pub extern "C" fn test_string_operations() -> c_int {
    let test_str = String::from("Hello, LoongScript!");
    let len = string_length(&test_str);
    len // Should return 19
}

#[no_mangle]
pub extern "C" fn test_vector_operations() -> c_int {
    let numbers: Vec<c_int> = vec![10, 20, 30, 40, 50];
    print_vector_sum(&numbers);
    numbers.len() as c_int // Should return 5
}

// Functions that accept strings and vectors from host via vmcall
#[no_mangle]
pub extern "C" fn process_message(msg: &String) -> c_int {
    let greeting = format!("Processing message: {}", msg);
    log_message(&greeting);
    msg.len() as c_int
}

#[no_mangle]
pub extern "C" fn sum_numbers(numbers: &Vec<c_int>) -> c_int {
    numbers.iter().sum()
}

#[no_mangle]
pub extern "C" fn process_dialogue(speaker: &String, scores: &Vec<c_int>) {
    let msg = format!("Speaker: {}", speaker);
    log_message(&msg);
    for score in scores {
        let score_msg = format!("  Score: {}", score);
        log_message(&score_msg);
    }
    print_vector_sum(scores);
}

// Complex nested datatypes
#[repr(C)]
pub struct Dialogue {
    speaker: String,
    lines: Vec<String>,
}

#[no_mangle]
pub extern "C" fn do_dialogue(dlg: &Dialogue) {
    let intro = format!("Dialogue by {}:", dlg.speaker);
    log_message(&intro);
    for line in &dlg.lines {
        let formatted = format!("  {}", line);
        log_message(&formatted);
    }
}

// Main function for standalone execution (if needed)
fn main() {
    println!(">>> Hello from the LoongScript Guest!");
    unsafe {
        fast_exit(0);
    }
}
