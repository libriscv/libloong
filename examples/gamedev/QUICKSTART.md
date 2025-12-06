# Quick Start Guide

Get the game running in 3 minutes!

## Prerequisites

- LoongArch GCC cross-compiler: `loongarch64-linux-gnu-gcc-14`
- Rust with LoongArch target
- CMake 3.22+

## Build & Run

```bash
# Just run it! Everything is automatic:
./run.sh
```

That's it! The script will:
- Build the host engine (C++) if needed
- Generate Rust API bindings
- Build the guest game (Rust) if needed
- Launch the game

## Controls

- **A** / **D** → Move ship left/right
- **Q** → Quit

## How It Works

1. **Host (C++)** handles rendering, input, and calls `game_update()` each frame
2. **Guest (Rust)** runs game logic: physics, collisions, spawning, etc.
3. **Guest calls host** via bindings: `draw_pixel()`, `get_delta_time()`, etc.

## File Overview

```
examples/gamedev/
├── main.cpp              # Host game engine (C++)
├── guest_game/
│   └── src/main.rs       # Guest game logic (Rust)
├── build.sh              # Build host
└── guest_game/build.sh   # Build guest
```

## Customize

Want to modify the game?

- **Change game speed**: Edit `spawn_rate` in [guest_game/src/main.rs](guest_game/src/main.rs)
- **Add new callbacks**: Register in `init_host_functions()` in [main.cpp](main.cpp)
- **Screen size**: Change `SCREEN_WIDTH`/`SCREEN_HEIGHT` constants

See [README.md](README.md) for full documentation.
