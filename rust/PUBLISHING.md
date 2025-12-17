# Publishing the Rust Bindings to crates.io

## Overview

The libloong Rust bindings require the C++ source code to build. Since cargo packages can only include files within their directory, we use a vendoring approach to prepare the package for publishing.

## Publishing Steps

1. **Vendor the C++ sources**:
   ```bash
   cd rust
   ./vendor-sources.sh
   ```

   This script copies the necessary C++ source files into a `_vendor/` directory within the rust crate. The vendored files include:
   - CMakeLists.txt (patched to remove tests subdirectory)
   - lib/ (the C++ library source)
   - LICENSE and README.md (required by CMake's CPack configuration)

2. **Test the package locally**:
   ```bash
   cargo package --allow-dirty
   ```

   This will create a package tarball and verify it builds correctly.

3. **Publish to crates.io**:
   ```bash
   cargo publish --allow-dirty
   ```

4. **Clean up** (optional):
   ```bash
   rm -rf _vendor
   ```

## How It Works

- **Development**: When building from the repository, `build.rs` looks for the C++ sources in the parent directory (`../lib/`, `../CMakeLists.txt`).

- **Published crate**: When building a downloaded crate from crates.io, `build.rs` finds the vendored sources in the `_vendor/` directory and uses those instead.

The `build.rs` automatically detects which mode to use, so the same build script works for both scenarios.

## Notes

- The `--allow-dirty` flag is needed because the vendored files are git-ignored.
- The vendored directory adds about 870 KB to the package size.
- The build.rs always prefers `_vendor/` over parent directory sources, so vendoring works even in the development tree.
