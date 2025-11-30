#![no_std]
#![no_main]

use core::panic::PanicInfo;

// Simple write syscall wrapper for LoongArch
#[inline(never)]
fn write(fd: i32, buf: &[u8]) -> isize {
    let ret: isize;
    unsafe {
        core::arch::asm!(
            "li.w $a7, 64",  // __NR_write = 64
            "syscall 0",
            in("$a0") fd,
            in("$a1") buf.as_ptr(),
            in("$a2") buf.len(),
            lateout("$a0") ret,
            lateout("$a7") _,
        );
    }
    ret
}

// Exit syscall wrapper for LoongArch
#[inline(never)]
fn exit(code: i32) -> ! {
    unsafe {
        core::arch::asm!(
            "li.w $a7, 93",  // __NR_exit = 93
            "syscall 0",
            in("$a0") code,
            options(noreturn)
        );
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    let message = b"Hello from Rust on LoongArch!\n";
    write(1, message);

    // Demonstrate some basic arithmetic
    let a: u64 = 42;
    let b: u64 = 13;
    let result = a + b;

    // Simple number to string conversion for demonstration
    let mut buf = [0u8; 32];
    let mut num = result;
    let mut idx = 0;

    if num == 0 {
        buf[idx] = b'0';
        idx += 1;
    } else {
        let mut digits = [0u8; 20];
        let mut digit_count = 0;

        while num > 0 {
            digits[digit_count] = (num % 10) as u8 + b'0';
            num /= 10;
            digit_count += 1;
        }

        for i in 0..digit_count {
            buf[idx] = digits[digit_count - 1 - i];
            idx += 1;
        }
    }

    write(1, b"Result: ");
    write(1, &buf[..idx]);
    write(1, b"\n");

    exit(0);
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    write(2, b"Panic occurred!\n");
    exit(1);
}
