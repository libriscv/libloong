#!/bin/bash
# Vendor C++ sources for cargo publishing
# This prepares the Rust crate to be published standalone

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBLOONG_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VENDOR_DIR="$SCRIPT_DIR/_vendor"

echo "Vendoring libloong C++ sources..."

# Remove old vendor directory if it exists
if [ -d "$VENDOR_DIR" ]; then
    echo "Removing old _vendor directory..."
    rm -rf "$VENDOR_DIR"
fi

# Create vendor directory
mkdir -p "$VENDOR_DIR"

# Copy and modify CMakeLists.txt to disable tests
echo "Copying and patching CMakeLists.txt..."
sed '/add_subdirectory(tests)/d' "$LIBLOONG_ROOT/CMakeLists.txt" > "$VENDOR_DIR/CMakeLists.txt"

# Copy lib directory
echo "Copying lib directory..."
cp -r "$LIBLOONG_ROOT/lib" "$VENDOR_DIR/"

# Copy LICENSE and README.md (needed by CMakeLists.txt CPack)
echo "Copying LICENSE and README.md..."
cp "$LIBLOONG_ROOT/LICENSE" "$VENDOR_DIR/"
cp "$LIBLOONG_ROOT/README.md" "$VENDOR_DIR/"

# Remove backup files
find "$VENDOR_DIR" -name "*.bak" -delete

echo ""
echo "Vendoring complete! The C++ sources are now in _vendor/"
echo ""
echo "You can now publish with:"
echo "  cargo publish --allow-dirty"
echo ""
echo "To clean up, simply: rm -rf _vendor"
