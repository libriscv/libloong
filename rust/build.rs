use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let libloong_root = manifest_dir.parent().unwrap();

    println!("cargo:rerun-if-changed=wrapper/libloong_wrapper.cpp");
    println!("cargo:rerun-if-changed=wrapper/libloong_wrapper.h");
    println!("cargo:rerun-if-changed=../lib/libloong");

    // Build the C++ wrapper library
    let mut build = cc::Build::new();
    build
        .cpp(true)
        .std("c++20")
        // NOTE: We need exceptions in the wrapper to catch C++ exceptions
        // and convert them to error codes for Rust. The wrapper acts as
        // an exception boundary - Rust code never sees C++ exceptions.
        .include(libloong_root.join("lib"))
        .include(libloong_root.join("build/lib")) // For libloong_settings.h
        .file("wrapper/libloong_wrapper.cpp");

    // Add optimization flags for release builds
    if !cfg!(debug_assertions) {
        build.opt_level(2);
    }

    // Link against the pre-built libloong static library
    println!(
        "cargo:rustc-link-search=native={}",
        libloong_root.join("build/lib").display()
    );
    println!("cargo:rustc-link-lib=static=loong");

    // Link against C++ standard library
    let target = env::var("TARGET").unwrap();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=c++");
    } else if target.contains("linux") {
        println!("cargo:rustc-link-lib=stdc++");
    }

    build.compile("loong_wrapper");

    println!("cargo:warning=Rust bindings for libloong compiled successfully");
}
