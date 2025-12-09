# Asteroid Dodge - LoongScript Game Example

A complete game development example demonstrating how to use the LoongScript framework to create a playable game with Rust-based game logic running on the LoongArch emulator.

## Overview

This example showcases:

- Host-side Game Engine (C++): Terminal-based renderer, input handling, and main game loop
- Guest-side Game Logic (Rust): Entity system, physics, collision detection, and game state
- Host Callbacks: Drawing functions, input queries, timing, and random number generation
- Real-time Scripting: Game logic runs at ~60 FPS with full access to host capabilities

## Architecture

```
┌─────────────────────────────────────────────────┐
│          Host (C++ Game Engine)                 │
│  - Terminal Renderer (ASCII graphics)           │
│  - Input System (non-blocking keyboard)         │
│  - Game Loop (60 FPS)                           │
│  - Host Callbacks (registered via HostBindings) │
└─────────────────┬───────────────────────────────┘
                  │ vmcall
                  │ (game_init, game_update)
                  ↓
┌─────────────────────────────────────────────────┐
│       Guest (Rust Game Logic)                   │
│  - Player Entity (position, velocity, input)    │
│  - Asteroid System (spawning, movement)         │
│  - Particle Effects (explosions)                │
│  - Collision Detection                          │
│  - Difficulty Scaling                           │
└─────────────────────────────────────────────────┘
```

## Host Callbacks

The host provides these functions to the guest:

### Drawing Functions
- `void draw_pixel(int x, int y, char c)` - Draw a character at screen position
- `void draw_text(int x, int y, const String& text)` - Draw text at position

### Input Functions
- `bool has_input()` - Check if keyboard input is available
- `char get_input()` - Get the last key pressed

### Game State Functions
- `int get_score()` - Get current score
- `void add_score(int points)` - Add points to score
- `void game_over()` - Signal game over

### Utility Functions
- `float get_delta_time()` - Get time since last frame (for smooth movement)
- `int random_int(int min, int max)` - Generate random number
- `float sin_f32(float x)` - Sine function
- `float cos_f32(float x)` - Cosine function
- `void log(const String& msg)` - Log debug messages

## Building and Running

### Quick Start

Just run the game! The script handles everything automatically:

```bash
cd examples/gamedev
./run.sh
```

This will:
1. Build the host engine (if needed)
2. Generate Rust API bindings
3. Build the Rust guest game (if needed)
4. Launch the game!

### Manual Build Steps

If you prefer to build components separately:

```bash
# 1. Build host manually
mkdir -p build && cd build
cmake .. && make -j$(nproc)
cd ..

# 2. Generate bindings
./asteroid_game --generate-bindings

# 3. Build guest
cd guest_game && ./build.sh && cd ..

# 4. Run
./asteroid_game
```

### Controls

- **A** or **Left Arrow**: Move left
- **D** or **Right Arrow**: Move right
- **Q** or **ESC**: Quit game

## Code Structure

### Host Side ([main.cpp](main.cpp))

```cpp
// Register host functions that guest can call
HostBindings::register_function(
    "void draw_pixel(int x, int y, char c)",
    [](loongarch::Machine&, int x, int y, char c) {
        GameEngine::draw_char(x, y, c);
    });

// Main game loop
while (game_state.running) {
    // Update timing
    calculate_delta_time();

    // Handle input
    process_keyboard();

    // Call guest update function
    game_script.call<void>("game_update");

    // Render to screen
    GameEngine::render();
}
```

### Guest Side ([guest_game/src/main.rs](guest_game/src/main.rs))

```rust
// Game entities
struct Player {
    pos: Vec2,
    velocity: f32,
}

struct Asteroid {
    pos: Vec2,
    velocity: Vec2,
    active: bool,
}

// Exported function called by host each frame
#[no_mangle]
pub extern "C" fn game_update() {
    let game = get_game_mut();

    // Process input
    if has_input() {
        handle_player_input(&mut game.player);
    }

    // Update entities
    game.player.update(get_delta_time());
    update_asteroids(&mut game.asteroids);

    // Check collisions
    if check_collisions(&game) {
        game_over();
    }

    // Render everything
    game.draw();
}
```

## Troubleshooting

**Game doesn't compile:**
- Ensure you have `loongarch64-linux-gnu-gcc-14` installed
- Check that Rust target `loongarch64-unknown-linux-gnu` is installed: `rustup target add loongarch64-unknown-linux-gnu`

**Game runs but crashes immediately:**
- Make sure you ran `--generate-bindings` before building the guest
- Verify `guest_game/game.elf` exists and was built recently
