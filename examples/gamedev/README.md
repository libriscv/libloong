# Asteroid Dodge - LoongScript Game Example

A complete game development example demonstrating how to use the LoongScript framework to create a playable game with Rust-based game logic running on the LoongArch emulator.

## Overview

This example showcases:

- **Host-side Game Engine** (C++): Terminal-based renderer, input handling, and main game loop
- **Guest-side Game Logic** (Rust): Entity system, physics, collision detection, and game state
- **Host Callbacks**: Drawing functions, input queries, timing, and random number generation
- **Real-time Scripting**: Game logic runs at ~60 FPS with full access to host capabilities

## Game Description

**Asteroid Dodge** is a simple arcade game where you control a spaceship and dodge falling asteroids:

- Control your ship with **A/D** keys or arrow keys
- Dodge asteroids as they fall from the sky
- Score points for each asteroid you successfully avoid
- Difficulty increases over time with faster asteroids and higher spawn rates
- Game over if you collide with an asteroid

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

## Key Features Demonstrated

### 1. Real-time Game Loop
The host runs a tight game loop at 60 FPS, calling into guest code each frame via `vmcall`.

### 2. Entity-Component System (in Rust)
Guest-side entities (Player, Asteroid, Particle) with composition and shared behavior.

### 3. Delta Time-based Movement
Smooth, framerate-independent movement using `get_delta_time()` callback.

### 4. Collision Detection
Simple AABB collision detection performed entirely in guest code.

### 5. Particle System
Explosion effects using velocity-based particles with lifetime management.

### 6. Progressive Difficulty
Difficulty scaling over time by adjusting spawn rates and speeds.

### 7. Non-blocking Input
Terminal configured for real-time input without blocking the game loop.

## Performance Considerations

- The game runs at ~60 FPS with negligible overhead from the emulator
- Each frame involves:
  - ~10-20 asteroid updates
  - Player physics update
  - Collision checks
  - Particle system updates
  - ASCII rendering (all drawing is buffered)
- The LoongArch bytecode system provides near-native performance for game logic
- Host callbacks have minimal overhead due to optimized dispatch

## Extending the Game

Some ideas for extending this example:

1. **Power-ups**: Add collectible items that give temporary abilities
2. **Shooting**: Let the player shoot projectiles to destroy asteroids
3. **Levels**: Create distinct levels with different layouts
4. **Enemies**: Add enemy ships with AI
5. **Audio**: Integrate audio callbacks for sound effects
6. **Better Graphics**: Use Unicode box-drawing characters or colors (ANSI codes)
7. **High Scores**: Persist scores to disk using file I/O callbacks
8. **Multiplayer**: Add a second player

## Learning Points

This example teaches:

- **How to structure a game** with LoongScript (host vs guest responsibilities)
- **Entity management** in Rust with safe patterns
- **Callback design** for game engine APIs
- **Real-time scripting** with predictable performance
- **Cross-language integration** (C++ host, Rust guest)
- **Game loop architecture** with fixed timestep rendering

## Troubleshooting

**Game doesn't compile:**
- Ensure you have `loongarch64-linux-gnu-gcc-14` installed
- Check that Rust target `loongarch64-unknown-linux-gnu` is installed: `rustup target add loongarch64-unknown-linux-gnu`

**Game runs but crashes immediately:**
- Make sure you ran `--generate-bindings` before building the guest
- Verify `guest_game/game.elf` exists and was built recently

**Input doesn't work:**
- Terminal must support non-blocking input (most Linux terminals do)
- Try running in a different terminal emulator

**Game is too fast/slow:**
- Check your system's performance
- Adjust the frame delay in [main.cpp](main.cpp:320) (currently 16ms for 60 FPS)

## Related Examples

- [examples/script](../script) - Basic LoongScript examples with C++ and Rust guests
- [examples/simple.cpp](../simple.cpp) - Minimal libloong usage

## License

Same as libloong project.
