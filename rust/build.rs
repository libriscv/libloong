use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    // Determine the libloong root directory
    // Priority: _vendor (for published crates) > parent directory (for development)
    let libloong_root = if manifest_dir.join("_vendor").exists() {
        // Published crate structure: sources are vendored in _vendor
        println!("cargo:warning=Using vendored sources from _vendor/");
        manifest_dir.join("_vendor")
    } else if let Some(parent) = manifest_dir.parent() {
        if parent.join("CMakeLists.txt").exists() && parent.join("lib").exists() {
            // Development structure: sources are in parent directory
            parent.to_path_buf()
        } else {
            panic!(
                "Cannot find libloong sources. \n\
                For publishing: run ./vendor-sources.sh first\n\
                For development: ensure this is in the libloong repository"
            );
        }
    } else {
        panic!("Cannot determine libloong root directory");
    };

    // Set rerun-if-changed for wrapper files
    println!("cargo:rerun-if-changed=wrapper/libloong_wrapper.cpp");
    println!("cargo:rerun-if-changed=wrapper/libloong_wrapper.h");

    // Watch for changes in the C++ library source (development mode only)
    if !manifest_dir.join("_vendor").exists() {
        println!("cargo:rerun-if-changed=../lib/libloong");
        println!("cargo:rerun-if-changed=../CMakeLists.txt");
    }

    // Build libloong C++ library using CMake
    let build_dir = out_dir.join("libloong_build");
    std::fs::create_dir_all(&build_dir).expect("Failed to create build directory");

    // Check if CMake is available
    let cmake_available = Command::new("cmake").arg("--version").output().is_ok();

    if !cmake_available {
        panic!("CMake is required to build libloong. Please install CMake and try again.");
    }

    // Check for stale CMake cache and clean if necessary
    let cmake_cache = build_dir.join("CMakeCache.txt");
    if cmake_cache.exists() {
        // Read the cache to check if the source directory has changed
        if let Ok(cache_content) = std::fs::read_to_string(&cmake_cache) {
            // Look for CMAKE_HOME_DIRECTORY in the cache
            let expected_source = libloong_root.canonicalize().ok();
            let mut cache_is_stale = false;

            for line in cache_content.lines() {
                if line.starts_with("CMAKE_HOME_DIRECTORY:") {
                    if let Some(cached_dir) = line.split('=').nth(1) {
                        let cached_path = PathBuf::from(cached_dir).canonicalize().ok();
                        if cached_path != expected_source {
                            println!("cargo:warning=Detected stale CMake cache (source dir changed), cleaning build directory");
                            cache_is_stale = true;
                            break;
                        }
                    }
                }
            }

            // Remove stale cache
            if cache_is_stale {
                std::fs::remove_dir_all(&build_dir).ok();
                std::fs::create_dir_all(&build_dir).expect("Failed to recreate build directory");
            }
        }
    }

    // Configure CMake
    let mut cmake_config = Command::new("cmake");
    cmake_config
        .current_dir(&build_dir)
        .arg(&libloong_root)
        .arg("-DCMAKE_BUILD_TYPE=Release");

    // Disable binary translation when building on docs.rs
    // (FetchContent is blocked in their sandbox)
    let is_docs_rs = env::var("DOCS_RS").is_ok();
    if is_docs_rs {
        println!("cargo:warning=Building for docs.rs: disabling binary translation");
        cmake_config.arg("-DLA_BINARY_TRANSLATION=OFF");
    } else {
        cmake_config.arg("-DLA_BINARY_TRANSLATION=ON");
    }

    // Use Ninja if available for faster builds
    if Command::new("ninja").arg("--version").output().is_ok() {
        cmake_config.arg("-GNinja");
    }

    let status = cmake_config
        .status()
        .expect("Failed to run CMake configuration");
    if !status.success() {
        panic!("CMake configuration failed");
    }

    // Build libloong
    let status = Command::new("cmake")
        .current_dir(&build_dir)
        .arg("--build")
        .arg(".")
        .arg("--target")
        .arg("loong")
        .arg("--config")
        .arg("Release")
        .arg("--parallel")
        .status()
        .expect("Failed to build libloong");

    if !status.success() {
        panic!("Failed to build libloong C++ library");
    }

    // Build the C++ wrapper library
    let mut build = cc::Build::new();
    build
        .cpp(true)
        .std("c++20")
        // NOTE: We need exceptions in the wrapper to catch C++ exceptions
        // and convert them to error codes for Rust. The wrapper acts as
        // an exception boundary - Rust code never sees C++ exceptions.
        .include(libloong_root.join("lib"))
        .include(build_dir.join("lib")) // For libloong_settings.h
        .file("wrapper/libloong_wrapper.cpp");

    // Add optimization flags for release builds
    if !cfg!(debug_assertions) {
        build.opt_level(2);
    }

    // Compile the wrapper
    build.compile("loong_wrapper");

    // Link against the libloong static library we just built
    println!(
        "cargo:rustc-link-search=native={}",
        build_dir.join("lib").display()
    );
    println!("cargo:rustc-link-lib=static=loong");

    // Link against C++ standard library
    let target = env::var("TARGET").unwrap();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=c++");
    } else if target.contains("linux") {
        println!("cargo:rustc-link-lib=stdc++");
    } else if target.contains("windows") {
        // Windows MSVC uses different C++ runtime
        println!("cargo:rustc-link-lib=msvcrt");
    }
}
