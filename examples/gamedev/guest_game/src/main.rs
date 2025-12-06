#![allow(unused)]
#![allow(dead_code)]
#![allow(non_camel_case_types)]

#[path = "../libloong_api.rs"]
mod libloong_api;
use libloong_api::*;

use std::ffi::c_int;

// Game constants
const SCREEN_WIDTH: i32 = 80;
const SCREEN_HEIGHT: i32 = 24;
const MAX_ASTEROIDS: usize = 10;
const PLAYER_Y: i32 = SCREEN_HEIGHT - 3;

// Entity types
#[derive(Clone, Copy)]
struct Vec2 {
    x: f32,
    y: f32,
}

impl Vec2 {
    fn new(x: f32, y: f32) -> Self {
        Vec2 { x, y }
    }
}

struct Player {
    pos: Vec2,
    velocity: f32,
    char_sprite: char,
}

impl Player {
    fn new() -> Self {
        Player {
            pos: Vec2::new((SCREEN_WIDTH / 2) as f32, PLAYER_Y as f32),
            velocity: 0.0,
            char_sprite: 'A',
        }
    }

    fn update(&mut self, delta_time: f32) {
        // Apply velocity
        self.pos.x += self.velocity * delta_time * 30.0;

        // Clamp to screen bounds (with border padding)
        self.pos.x = self.pos.x.max(2.0).min((SCREEN_WIDTH - 3) as f32);

        // Apply friction
        self.velocity *= 0.85;
    }

    fn move_left(&mut self) {
        //self.velocity = -1.0;
		self.pos.x -= 1.0;
    }

    fn move_right(&mut self) {
        //self.velocity = 1.0;
		self.pos.x += 1.0;
    }

    fn draw(&self) {
        draw_pixel(self.pos.x as i32, self.pos.y as i32, self.char_sprite as i8);
        // Draw ship body
        draw_pixel(self.pos.x as i32 - 1, self.pos.y as i32, b'/' as i8);
        draw_pixel(self.pos.x as i32 + 1, self.pos.y as i32, b'\\' as i8);
    }
}

struct Asteroid {
    pos: Vec2,
    velocity: Vec2,
    active: bool,
    char_sprite: char,
    rotation_time: f32,
}

impl Asteroid {
    fn new() -> Self {
        Asteroid {
            pos: Vec2::new(0.0, 0.0),
            velocity: Vec2::new(0.0, 0.0),
            active: false,
            char_sprite: '*',
            rotation_time: 0.0,
        }
    }

    fn spawn(&mut self, x: f32, speed: f32) {
        self.pos.x = x;
        self.pos.y = 2.0;  // Start below HUD
        self.velocity.x = (random_int(-10, 10) as f32) * 0.1;
        self.velocity.y = speed;
        self.active = true;
        self.rotation_time = 0.0;

        // Vary asteroid appearance
        let sprite_choice = random_int(0, 3);
        self.char_sprite = match sprite_choice {
            0 => '*',
            1 => 'o',
            2 => 'O',
            _ => '@',
        };
    }

    fn update(&mut self, delta_time: f32) {
        if !self.active {
            return;
        }

        self.pos.x += self.velocity.x * delta_time * 10.0;
        self.pos.y += self.velocity.y * delta_time * 10.0;

        // Animate rotation
        self.rotation_time += delta_time * 4.0;

        // Deactivate if out of bounds
		// XXX: f32 fails here
        if self.pos.y >= (SCREEN_HEIGHT - 1) as f32 {
            self.active = false;
            // Player dodged successfully
            add_score(10);
        }
    }

    fn draw(&self) {
        if !self.active {
            return;
        }

        let x = self.pos.x as i32;
        let y = self.pos.y as i32;

        // Draw asteroid with rotation effect
        let phase = (self.rotation_time.sin() * 2.0) as i32;
        match phase {
            -2 => {
                draw_pixel(x, y, self.char_sprite as u8 as i8);
            }
            -1 => {
                draw_pixel(x - 1, y, b'.' as i8);
                draw_pixel(x, y, self.char_sprite as u8 as i8);
                draw_pixel(x + 1, y, b'.' as i8);
            }
            0 | 1 => {
                draw_pixel(x - 1, y, self.char_sprite as u8 as i8);
                draw_pixel(x + 1, y, self.char_sprite as u8 as i8);
            }
            _ => {
                draw_pixel(x, y, self.char_sprite as u8 as i8);
            }
        }
    }

    fn check_collision(&self, player: &Player) -> bool {
        if !self.active {
            return false;
        }

        let dx = (self.pos.x - player.pos.x).abs();
        let dy = (self.pos.y - player.pos.y).abs();

        dx < 2.0 && dy < 1.5
    }
}

// Particle effects for explosions
struct Particle {
    pos: Vec2,
    velocity: Vec2,
    lifetime: f32,
    active: bool,
}

impl Particle {
    fn new() -> Self {
        Particle {
            pos: Vec2::new(0.0, 0.0),
            velocity: Vec2::new(0.0, 0.0),
            lifetime: 0.0,
            active: false,
        }
    }

    fn spawn(&mut self, x: f32, y: f32) {
        self.pos.x = x;
        self.pos.y = y;
        let angle = (random_int(0, 360) as f32) * 3.14159 / 180.0;
        let speed = (random_int(5, 15) as f32) * 0.1;
        self.velocity.x = cos_f32(angle) * speed;
        self.velocity.y = sin_f32(angle) * speed;
        self.lifetime = 2.5;
        self.active = true;
    }

    fn update(&mut self, delta_time: f32) {
        if !self.active {
            return;
        }

        self.pos.x += self.velocity.x * delta_time * 10.0;
        //self.pos.y += self.velocity.y * delta_time * 10.0;
		self.pos.y += 0.2; // particles fall downwards
        self.lifetime -= delta_time;

        if self.lifetime <= 0.0 {
            self.active = false;
        }
    }

    fn draw(&self) {
        if !self.active {
            return;
        }

        let c = if self.lifetime > 0.3 { b'*' as i8 } else { b'.' as i8 };
        draw_pixel(self.pos.x as i32, self.pos.y as i32, c);
    }
}

// Global game state
static mut GAME: Option<Game> = None;

struct Game {
    player: Player,
    asteroids: [Asteroid; MAX_ASTEROIDS],
    particles: Vec<Particle>,
    spawn_timer: f32,
    spawn_rate: f32,
    difficulty: f32,
    frames: u32,
}

impl Game {
    fn new() -> Self {
        Game {
            player: Player::new(),
            asteroids: core::array::from_fn(|_| Asteroid::new()),
            particles: Vec::new(),
            spawn_timer: 0.0,
            spawn_rate: 1.5,
            difficulty: 1.0,
            frames: 0,
        }
    }

    fn spawn_asteroid(&mut self) {
        // Find inactive asteroid
        for asteroid in &mut self.asteroids {
            if !asteroid.active {
                let x = random_int(5, SCREEN_WIDTH - 5) as f32;
                let speed = 1.0 + (self.difficulty * 0.5);
                asteroid.spawn(x, speed);
                break;
            }
        }
    }

    fn spawn_explosion(&mut self, x: f32, y: f32) {
        // Spawn multiple particles
        for _ in 0..8 {
            let mut particle = Particle::new();
            particle.spawn(x, y);
            self.particles.push(particle);
        }
    }

    fn update(&mut self, delta_time: f32) {
        self.frames += 1;
		//log(&format!("Frame: {} Delta time: {:.2}", self.frames, delta_time));

        // Handle input
        if has_input() {
            let input = get_input() as char;
            // Input constants
            const A_LOWER: char = 'a';
            const A_UPPER: char = 'A';
            const H: char = 'h';
            const D_LOWER: char = 'd';
            const D_UPPER: char = 'D';
            const L: char = 'l';

            match input {
                A_LOWER | A_UPPER | H => self.player.move_left(),
                D_LOWER | D_UPPER | L => self.player.move_right(),
                _ => {}
            }
        }

        // Update player
        self.player.update(delta_time);

        // Update asteroids and check collisions
        let player_x = self.player.pos.x;
        let player_y = self.player.pos.y;
        let mut collision = false;

        for asteroid in &mut self.asteroids {
            asteroid.update(delta_time);

            // Check collision with player
            if asteroid.check_collision(&self.player) {
                collision = true;
            }
        }

        if collision {
            self.spawn_explosion(player_x, player_y);
            game_over();
        }

        // Update particles
        self.particles.retain_mut(|p| {
            p.update(delta_time);
            p.active
        });

        // Spawn asteroids
        self.spawn_timer += delta_time;
        if self.spawn_timer >= self.spawn_rate {
            self.spawn_timer = 0.0;
            self.spawn_asteroid();
        }

        // Increase difficulty over time
        if self.frames % 300 == 0 {
            self.difficulty += 0.1;
            self.spawn_rate = (self.spawn_rate * 0.95).max(0.3);
        }
    }

    fn draw(&self) {
        // Draw player
        self.player.draw();

        // Draw asteroids
        for asteroid in &self.asteroids {
            asteroid.draw();
        }

        // Draw particles
        for particle in &self.particles {
            particle.draw();
        }

        // Draw HUD
        let score = get_score();
        let score_text = format!("Score: {}", score);
        draw_text(3, 1, &score_text);

        let diff_text = format!("Difficulty: {:.1}", self.difficulty);
        draw_text(SCREEN_WIDTH - 20, 1, &diff_text);

        // Draw controls hint
        draw_text(3, SCREEN_HEIGHT - 2, &String::from("A/D: Move  Q: Quit"));
    }
}

// Exported functions called by host

#[no_mangle]
pub extern "C" fn game_init() {
    unsafe {
        GAME = Some(Game::new());
    }
    //log(&String::from("Game initialized"));
}

#[no_mangle]
pub extern "C" fn game_update() {
    unsafe {
        if let Some(game) = &mut GAME {
            let delta_time = get_delta_time();
            game.update(delta_time);
            game.draw();
        }
    }
}

// Main function
fn main() {
    println!(">>> Asteroid Dodge Game - Guest Module");
    unsafe {
        fast_exit(0);
    }
}
