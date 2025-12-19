#!/bin/bash
set -e

echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║           Asteroid Dodge - LoongScript Game - Build & Run                ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""

echo "[1/4] Building host game engine..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DLA_BINARY_TRANSLATION=ON > /dev/null
make -j$(nproc)
cd ..
echo "      ✓ Host engine built"

# Step 2: Generate API bindings
echo "[2/4] Generating Rust API bindings..."
./asteroid_game --generate-bindings > /dev/null 2>&1
echo "      ✓ API bindings generated"

# Step 3: Build guest game
REBUILD_GUEST=true
echo "[3/4] Building Rust guest game..."

if [ "$REBUILD_GUEST" = true ]; then
    cd guest_game
    ./build.sh
    cd ..
    echo "      ✓ Guest game built"
fi

# Step 4: Run the game
echo "[4/4] Launching game..."
echo ""
echo "════════════════════════════════════════════════════════════════════════════"
echo ""

./asteroid_game "$@"
