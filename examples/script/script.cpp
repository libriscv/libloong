#include "script.hpp"
#include <libloong/decoder_cache.hpp>
#include <libloong/threaded_bytecodes.hpp>
#include <fmt/core.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <array>

namespace loongarch::script {

// Helper function to execute a shell command and capture output
static std::pair<int, std::string> execute_command(const std::string& command) {
	std::array<char, 256> buffer;
	std::string result;
	int exit_code = -1;

	FILE* pipe = popen(command.c_str(), "r");
	if (!pipe) {
		return {-1, "Failed to execute command"};
	}

	while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
		result += buffer.data();
	}

	exit_code = pclose(pipe);
	return {WEXITSTATUS(exit_code), result};
}

// Constructor: Load from existing ELF binary
Script::Script(const std::string& elf_path, const ScriptOptions& options)
	: m_userdata(options.userdata),
	  m_binary(load_elf_file(elf_path)),
	  m_options(options)
{
	initialize_machine();
}

// Internal constructor from binary data
Script::Script(std::vector<uint8_t> binary, const ScriptOptions& options)
	: m_userdata(options.userdata),
	  m_binary(std::move(binary)),
	  m_options(options)
{
	initialize_machine();
}

// Destructor
Script::~Script() {
	if (!m_temp_file.empty() && !m_options.keep_temp_files) {
		std::filesystem::remove(m_temp_file);
	}
}

// Static factory: Compile from C++ source code
Script Script::from_source(const std::string& source_code, const ScriptOptions& options) {
	// Auto-prepend fast_exit function and dummy main for vmcall support
	std::string preamble = R"(
#include <string>
#include <vector>

__asm__(
	".pushsection .text\n"
	".global fast_exit\n"
	".type fast_exit, @function\n"
	"fast_exit:\n"
	"  move $zero, $zero\n"
	".popsection\n"
);
extern "C" __attribute__((noreturn)) void fast_exit(int code);

int main() { fast_exit(0); }

#define HOST(name, type) \
	asm(".pushsection .text\n" \
	".global " #name "\n" \
	".type " #name ", @function\n" \
	 #name ":\n" \
	"  ret\n" \
	".popsection\n"); \
	extern "C" { type; }

extern "C" {
)";

	// Inject user-defined header content
	preamble += HostBindings::get_header();

	// Inject all registered host function declarations
	preamble += HostBindings::generate_extern_declarations();
	preamble += "}\n\n";

	// Inject assembly stubs for all registered host functions
	preamble += HostBindings::generate_asm_stubs();
	preamble += "\n";

	const std::string full_source = preamble + source_code;

	std::string temp_file;
	auto binary = compile_source(full_source, options, temp_file);

	Script script(std::move(binary), options);
	script.m_temp_file = std::move(temp_file);
	return script;
}

// Static factory: Compile from C++ source file
Script Script::from_file(const std::string& source_path, const ScriptOptions& options) {
	std::ifstream file(source_path);
	if (!file) {
		throw ScriptException("Failed to open source file: " + source_path);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return from_source(buffer.str(), options);
}

// Compile C++ source to ELF binary
std::vector<uint8_t> Script::compile_source(
	const std::string& source_code,
	const ScriptOptions& options,
	std::string& temp_filepath)
{
	// Create temporary source file
	std::string source_file = options.temp_dir + "/script_XXXXXX.cpp";
	int fd = mkstemps(const_cast<char*>(source_file.data()), 4);
	if (fd < 0) {
		throw CompilationException("Failed to create temporary source file");
	}

	// Write source code to file
	ssize_t written = write(fd, source_code.data(), source_code.size());
	close(fd);

	if (written < 0 || static_cast<size_t>(written) != source_code.size()) {
		std::filesystem::remove(source_file);
		throw CompilationException("Failed to write source code to temporary file");
	}

	// Create output file path
	std::string output_file = options.temp_dir + "/script_XXXXXX.elf";
	fd = mkstemps(const_cast<char*>(output_file.data()), 4);
	if (fd < 0) {
		std::filesystem::remove(source_file);
		throw CompilationException("Failed to create temporary output file");
	}
	close(fd);

	// Build compilation command
	std::ostringstream cmd;
	cmd << options.compiler << " ";

	// Add compile flags
	for (const auto& flag : options.compile_flags) {
		cmd << flag << " ";
	}

	// Add link flags
	for (const auto& flag : options.link_flags) {
		cmd << flag << " ";
	}

	// Add input and output files
	cmd << source_file << " -o " << output_file;

	if (options.verbose) {
		cmd << " 2>&1";
	} else {
		cmd << " 2>&1";
	}

	// Execute compilation
	auto [exit_code, output] = execute_command(cmd.str());

	// Clean up source file
	if (!options.keep_temp_files) {
		std::filesystem::remove(source_file);
	}

	// Check compilation result
	if (exit_code != 0) {
		std::filesystem::remove(output_file);
		throw CompilationException(
			"Compilation failed with exit code " + std::to_string(exit_code) +
			":\n" + output
		);
	}

	if (options.verbose && !output.empty()) {
		fmt::print("Compilation output:\n{}\n", output);
	}

	// Load the compiled binary
	temp_filepath = output_file;
	return load_elf_file(output_file);
}

// Load ELF file into memory
std::vector<uint8_t> Script::load_elf_file(const std::string& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw ScriptException("Failed to open ELF file: " + path);
	}

	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
		throw ScriptException("Failed to read ELF file: " + path);
	}

	return buffer;
}

// Initialize the machine
void Script::initialize_machine() {
	MachineOptions m_machineoptions{
		.memory_max = m_options.memory_max,
		.stack_size = m_options.stack_size,
		.brk_size = m_options.brk_size,
		.verbose_loader = m_options.verbose,
		.verbose_syscalls = m_options.verbose,
	};

	try {
		// Create machine instance
		m_machine = std::make_unique<Machine>(m_binary, m_machineoptions);
		m_machine->set_userdata(this);

		// Setup Linux environment
		std::vector<std::string> args = {"script"};
		std::vector<std::string> env = {"LC_ALL=C"};
		m_machine->setup_linux(args, env);

		// Setup syscalls and threads
		m_machine->setup_linux_syscalls();
		m_machine->setup_posix_threads();
		m_machine->setup_accelerated_syscalls();

		static constexpr size_t HEAP_SIZE = 16ull << 20; // 16 MB
		address_t heap_addr = m_machine->memory.mmap_allocate(HEAP_SIZE);
		m_machine->setup_accelerated_heap(heap_addr, HEAP_SIZE);

		// Try to find and set the fast_exit address for vmcall
		auto exit_addr = m_machine->address_of("fast_exit");
		m_machine->memory.set_exit_address(exit_addr);
		// fast_exit not found - vmcall will not work
		if (exit_addr == 0 && m_options.verbose) {
			fmt::print(stderr, "Warning: fast_exit function not found. "
							"vmcall functionality will not work.\n");
		}

		// Setup unknown syscall handler for host callbacks
		Machine::set_unknown_syscall_handler([](Machine& machine, int sysnum) {
			Script::dispatch_callback(machine, sysnum);
		});

		// Patch all registered host functions
		patch_host_functions();

		if (m_options.max_instructions == 0) {
			m_machine->cpu.simulate_inaccurate(m_machine->cpu.pc());
		} else {
			m_machine->simulate(m_options.max_instructions);
		}

		// Create new stack for VM calls
		auto stack = m_machine->memory.mmap_allocate(m_options.stack_size);
		m_machine->memory.set_stack_address(stack + m_options.stack_size);

	} catch (const MachineException& e) {
		fmt::print(stderr, "Machine initialization error: {} Data 0x{:x}\n",
			e.what(), e.data());
		throw ScriptException("Failed to initialize machine: " + std::string(e.what()));
	}
}

void Script::handle_exception(const MachineException& e) const
{
	fmt::print(stderr, "Machine exception: {} Data 0x{:x}\n",
		e.what(), e.data());
	throw ScriptException(
		"Machine exception (" + std::to_string(static_cast<int>(e.type())) +
		"): " + e.what()
	);
}

bool Script::has_function(const std::string& function_name) const
{
	try {
		m_machine->address_of(function_name);
		return true;
	} catch (...) {
		return false;
	}
}

// Get address of a symbol
address_t Script::address_of(const std::string& symbol_name) const
{
	try {
		return m_machine->address_of(symbol_name);
	} catch (const std::exception& e) {
		throw ScriptException("Symbol not found: " + symbol_name);
	}
}

// Dispatch callback to registered handler (static)
void Script::dispatch_callback(Machine& machine, int syscall_num) {
	HostBindings::dispatch(machine, syscall_num);
}

// Patch all registered host functions into the guest
void Script::patch_host_functions()
{
	for (const auto& [name, binding] : HostBindings::get_bindings()) {
		// Check if function exists in guest
		try {
			auto addr = address_of(name);
			if (addr == 0) {
				if (m_options.verbose) {
					fmt::print(stderr,
						"Warning: Host function '{}' is missing in guest, skipping\n", name);
				}
				continue;
			}

			if (m_options.verbose) {
				fmt::print("Patching host function '{}' at address 0x{:x} to syscall {}\n",
					name, addr, binding.syscall_num);
			}

			// Create decoder cache entry for SYSCALLIMM bytecode
			DecoderData entry;
			entry.bytecode = LA64_BC_SYSCALLIMM;
			entry.handler_idx = 0; // Invalid
			entry.block_bytes = 0; // Diverges here
			entry.instr = binding.syscall_num;

			// Install into decoder cache
			auto exec_seg = m_machine->memory.exec_segment_for(addr);
			if (!exec_seg->empty()) {
				exec_seg->set(addr, entry);
			}

		} catch (const ScriptException&) {
			// Function not found in guest - skip silently unless verbose
			if (m_options.verbose) {
				fmt::print("Warning: Host function '{}' not found in guest, skipping\n", name);
			}
		}
	}
}

} // loongarch::script
