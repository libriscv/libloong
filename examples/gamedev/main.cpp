#include <script.hpp>
#include <fmt/core.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <iostream>
using namespace loongarch::script;

// Simple ASCII-based terminal game renderer
namespace GameEngine {
	constexpr int SCREEN_WIDTH = 80;
	constexpr int SCREEN_HEIGHT = 24;
	char screen_buffer[SCREEN_HEIGHT][SCREEN_WIDTH + 1];

	void clear_screen() {
		fmt::print("\033[2J\033[H"); // ANSI escape codes to clear screen
	}

	void init_buffer() {
		for (int y = 0; y < SCREEN_HEIGHT; y++) {
			for (int x = 0; x < SCREEN_WIDTH; x++) {
				screen_buffer[y][x] = ' ';
			}
			screen_buffer[y][SCREEN_WIDTH] = '\0';
		}
	}

	void draw_char(int x, int y, char c) {
		if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
			screen_buffer[y][x] = c;
		}
	}

	void draw_border() {
		// Top and bottom borders
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			screen_buffer[0][x] = '=';
			screen_buffer[SCREEN_HEIGHT - 1][x] = '=';
		}
		// Side borders
		for (int y = 0; y < SCREEN_HEIGHT; y++) {
			screen_buffer[y][0] = '|';
			screen_buffer[y][SCREEN_WIDTH - 1] = '|';
		}
		// Corners
		screen_buffer[0][0] = '+';
		screen_buffer[0][SCREEN_WIDTH - 1] = '+';
		screen_buffer[SCREEN_HEIGHT - 1][0] = '+';
		screen_buffer[SCREEN_HEIGHT - 1][SCREEN_WIDTH - 1] = '+';
	}

	void render(double t, uint64_t cycles) {
		clear_screen();
		for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
			fmt::print("{}\n{}\n", screen_buffer[y+0], screen_buffer[y+1]);
		}
		// One line up, display stats
		fmt::print("\033[{}A", 1);
		fmt::print("+= Time: {:.2f}us  Instr: {}  MI/s: {:.2f} ==\n",
			t * 1e6,
			cycles,
			(cycles / 1e6) / t);
	}
}

// Game state managed by host
struct HostGameState {
	bool running = true;
	int score = 0;
	float delta_time = 0.033f; // ~30 FPS
	std::chrono::steady_clock::time_point last_frame_time;

	// Input state
	char last_input = '\0';
	bool input_available = false;
};

static HostGameState game_state;
#define HFN HostBindings::register_function

// Host callback functions that the guest can call
void init_host_functions() {
	// Drawing functions
	HFN("void draw_pixel(int x, int y, char c)",
		[](loongarch::Machine&, int x, int y, char c) {
			GameEngine::draw_char(x, y, c);
		});

	HFN("void draw_text(int x, int y, const std::string& text)",
		[](loongarch::Machine& machine, int x, int y, const loongarch::GuestRustString* text) {
			auto view = text->to_view(machine);
			int pos = 0;
			for (char c : view) {
				if (x + pos < GameEngine::SCREEN_WIDTH) {
					GameEngine::draw_char(x + pos, y, c);
				}
				pos++;
			}
		});

	// Input functions
	HFN("bool has_input()",
		[](loongarch::Machine&) -> bool {
			return game_state.input_available;
		});

	HFN("uint8_t get_input()",
		[](loongarch::Machine&) -> char {
			char input = game_state.last_input;
			game_state.input_available = false;
			game_state.last_input = '\0';
			return input;
		});

	// Time functions
	HFN("float get_delta_time()",
		[](loongarch::Machine&) -> float {
			return game_state.delta_time;
		});

	// Game state functions
	HFN("int get_score()",
		[](loongarch::Machine&) -> int {
			return game_state.score;
		});

	HFN("void add_score(int points)",
		[](loongarch::Machine&, int points) {
			game_state.score += points;
		});

	HFN("void game_over()",
		[](loongarch::Machine&) {
			game_state.running = false;
		});

	// Utility functions
	HFN("int random_int(int min, int max)",
		[](loongarch::Machine&, int min, int max) -> int {
			return min + (rand() % (max - min + 1));
		});

	HFN("float sin_f32(float x)",
		[](loongarch::Machine&, float x) -> float {
			return std::sin(x);
		});

	HFN("float cos_f32(float x)",
		[](loongarch::Machine&, float x) -> float {
			return std::cos(x);
		});

	HFN("void log(const std::string& msg)",
		[](loongarch::Machine& machine, const loongarch::GuestRustString* msg) {
			FILE* log_file = fopen("game_log.txt", "a");
			if (log_file) {
				fmt::print(log_file, "[GAME LOG] {}\n", msg->to_view(machine));
				fclose(log_file);
			}
		});
}

// Non-blocking input (Linux terminal)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static void set_nonblocking_input(bool enable) {
	static struct termios old_tio, new_tio;
	if (enable) {
		tcgetattr(STDIN_FILENO, &old_tio);
		new_tio = old_tio;
		new_tio.c_lflag &= (~ICANON & ~ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
		fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	} else {
		tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
		fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
	}
}

static char get_key_press() {
	char c = 0;
	if (read(STDIN_FILENO, &c, 1) == 1) {
		return c;
	}
	return '\0';
}

void print_usage(const char* program_name) {
	fmt::print("Usage: {} [OPTIONS]\n\n", program_name);
	fmt::print("Options:\n");
	fmt::print("  --generate-bindings    Generate API bindings for Rust guest project\n");
	fmt::print("  -v, --verbose          Enable verbose output\n");
	fmt::print("  -h, --help             Show this help message\n\n");
	fmt::print("Controls:\n");
	fmt::print("  A/D or Arrow Keys      Move left/right\n");
	fmt::print("  Q                      Quit game\n");
}

int main(int argc, char* argv[]) {
	bool verbose = false;
	bool generate_bindings = false;

	// Parse command line arguments
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			print_usage(argv[0]);
			return 0;
		} else if (arg == "-v" || arg == "--verbose") {
			verbose = true;
		} else if (arg == "--generate-bindings") {
			generate_bindings = true;
		} else {
			fmt::print(stderr, "Error: Unknown option '{}'\n\n", arg);
			print_usage(argv[0]);
			return 1;
		}
	}

	init_host_functions();

	// Generate bindings if requested
	if (generate_bindings) {
		fmt::print("Generating API bindings for Rust game...\n");
		const std::filesystem::path rust_api_path = "guest_game/libloong_api.rs";
		const std::filesystem::path rust_src_path = "guest_game/src";
		APIGenerator::write_rust_api(rust_api_path, rust_src_path);
		fmt::print("  Rust API: {}\n", rust_api_path.string());
		fmt::print("\nAPI generation complete!\n");
		fmt::print("Build the game with:\n");
		fmt::print("  cd guest_game && chmod +x build.sh && ./build.sh\n");
		return 0;
	}

	const std::string guest_path = "guest_game/game.elf";
	if (!std::filesystem::exists(guest_path)) {
		fmt::print(stderr, "Error: Game executable not found: {}\n", guest_path);
		fmt::print(stderr, "Run with --generate-bindings first, then build the guest game.\n");
		return 1;
	}

	fmt::print("╔═══════════════════════════════════════════════════════════════════════════╗\n");
	fmt::print("║                    ASTEROID DODGE - LoongScript Game                      ║\n");
	fmt::print("╚═══════════════════════════════════════════════════════════════════════════╝\n\n");
	fmt::print("Loading game from: {}\n", guest_path);
	fmt::print("Controls: A/D or Arrow Keys to move, Q to quit\n");
	fmt::print("Press any key to start...\n");

	std::cin.get();

	ScriptOptions options;
	options.verbose = verbose;
	bool was_binary_translated = false;

	try {
		Script game_script(guest_path, options);
		was_binary_translated = game_script.machine().is_binary_translation_enabled();
		if (!game_script.has_function("game_init") ||
			!game_script.has_function("game_update")) {
			fmt::print(stderr, "Error: Guest game is missing required functions (game_init, game_update)\n");
			exit(1);
		}

		// Initialize the game (guest-side)
		fmt::print("Initializing game...\n");
		game_script.call<void>("game_init");

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		// Set up terminal for game
		set_nonblocking_input(true);

		game_state.last_frame_time = std::chrono::steady_clock::now();

		// Main game loop
		while (game_state.running) {
			// Calculate delta time
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<float> elapsed = now - game_state.last_frame_time;
			game_state.delta_time = elapsed.count();
			game_state.last_frame_time = now;

			// Handle input
			const char key = get_key_press();
			if (key != '\0') {
				if (key == 'q' || key == 'Q' || key == 27) { // ESC
					break;
				}
				game_state.last_input = key;
				game_state.input_available = true;
			}

			// Clear and prepare buffer
			GameEngine::init_buffer();
			GameEngine::draw_border();

			// Update game logic (guest-side)
			auto start_time = std::chrono::steady_clock::now();
			game_script.call<void>("game_update");
			auto cycles = game_script.machine().instruction_counter();
			auto end_time = std::chrono::steady_clock::now();

			// Render
			GameEngine::render(
				std::chrono::duration<double>(end_time - start_time).count(),
				cycles);

			// Target 30 FPS
			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}

		// Cleanup terminal
		set_nonblocking_input(false);

		// Game over screen
		GameEngine::clear_screen();
		fmt::print("\n╔═══════════════════════════════════════════════════════════════════════════╗\n");
		fmt::print("║                              GAME OVER                                    ║\n");
		fmt::print("╚═══════════════════════════════════════════════════════════════════════════╝\n");
		fmt::print("\nFinal Score: {}  Binary translation: {}\n",
			game_state.score,
			was_binary_translated ? "Enabled" : "Disabled");
		fmt::print("\nThanks for playing!\n");

	} catch (const std::exception& e) {
		set_nonblocking_input(false);
		fmt::print(stderr, "Error: {}\n", e.what());
		return 1;
	}

	return 0;
}
