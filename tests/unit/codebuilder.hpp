#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <array>

namespace loongarch::test {

struct CompilerOptions {
	bool static_linking = true;
	bool nostdlib = false;
	bool nostartfiles = false;
	int optimization = 0;  // -O0, -O1, -O2, -O3
	bool debug_info = true;
	std::string text_segment = "0x200000";
	std::vector<std::string> extra_flags;
	std::vector<std::string> defines;
	std::vector<std::string> include_paths;
	std::string output_dir = "/tmp/loongarch_tests";
};

class CodeBuilder {
public:
	CodeBuilder() {
		// Find the LoongArch compiler
		const char* search_names[] = {
			"loongarch64-linux-gnu-gcc-14",
			"loongarch64-linux-gnu-gcc",
			"loongarch64-unknown-linux-gnu-gcc",
			nullptr
		};

		for (const char** name = search_names; *name != nullptr; ++name) {
			if (find_program(*name)) {
				m_compiler = *name;
				break;
			}
		}

		if (m_compiler.empty()) {
			throw std::runtime_error(
				"LoongArch compiler not found. Please install loongarch64-linux-gnu-gcc-14 or loongarch64-linux-gnu-gcc");
		}
	}

	// Compile C source code to LoongArch binary
	std::vector<uint8_t> build(
		const std::string& source_code,
		const std::string& name = "test_program",
		const CompilerOptions& opts = {})
	{
		// Create output directory if it doesn't exist
		std::filesystem::create_directories(opts.output_dir);

		// Write source to temporary file
		std::string source_path = opts.output_dir + "/" + name + ".c";
		std::string output_path = opts.output_dir + "/" + name + ".elf";

		std::ofstream source_file(source_path);
		if (!source_file) {
			throw std::runtime_error("Failed to create source file: " + source_path);
		}
		source_file << source_code;
		source_file.close();

		// Build compiler command
		std::ostringstream cmd;
		cmd << m_compiler;

		if (opts.static_linking) cmd << " -static";
		if (opts.nostdlib) cmd << " -nostdlib";
		if (opts.nostartfiles) cmd << " -nostartfiles";
		if (opts.debug_info) cmd << " -g";

		cmd << " -O" << opts.optimization;

		// Add defines
		for (const auto& def : opts.defines) {
			cmd << " -D" << def;
		}

		// Add include paths
		for (const auto& inc : opts.include_paths) {
			cmd << " -I" << inc;
		}

		// Add text segment linker flag
		if (!opts.text_segment.empty()) {
			cmd << " -Wl,-Ttext-segment=" << opts.text_segment;
		}

		// Add extra flags
		for (const auto& flag : opts.extra_flags) {
			cmd << " " << flag;
		}

		cmd << " " << source_path;
		cmd << " -o " << output_path;
		cmd << " 2>&1";  // Capture stderr

		// Execute compiler
		std::string command = cmd.str();
		std::string output = exec(command);

		// Check if compilation succeeded
		if (!std::filesystem::exists(output_path)) {
			throw std::runtime_error(
				"Compilation failed for " + name + ":\n" +
				"Command: " + command + "\n" +
				"Output: " + output);
		}

		// Read the compiled binary
		return read_binary(output_path);
	}

	// Compile C++ source code
	std::vector<uint8_t> build_cpp(
		const std::string& source_code,
		const std::string& name = "test_program",
		const CompilerOptions& opts = {})
	{
		// Create output directory if it doesn't exist
		std::filesystem::create_directories(opts.output_dir);

		// Write source to temporary file with .cpp extension
		std::string source_path = opts.output_dir + "/" + name + ".cpp";
		std::string output_path = opts.output_dir + "/" + name + ".elf";

		std::ofstream source_file(source_path);
		if (!source_file) {
			throw std::runtime_error("Failed to create source file: " + source_path);
		}
		source_file << source_code;
		source_file.close();

		// Build compiler command using g++
		std::string cpp_compiler = m_compiler;
		// Replace gcc with g++
		size_t pos = cpp_compiler.find("gcc");
		if (pos != std::string::npos) {
			cpp_compiler.replace(pos, 3, "g++");
		}

		std::ostringstream cmd;
		cmd << cpp_compiler;

		if (opts.static_linking) cmd << " -static";
		if (opts.nostdlib) cmd << " -nostdlib";
		if (opts.nostartfiles) cmd << " -nostartfiles";
		if (opts.debug_info) cmd << " -g";

		cmd << " -O" << opts.optimization;
		cmd << " -std=c++17";  // Default to C++17

		// Add defines
		for (const auto& def : opts.defines) {
			cmd << " -D" << def;
		}

		// Add include paths
		for (const auto& inc : opts.include_paths) {
			cmd << " -I" << inc;
		}

		// Add text segment linker flag
		if (!opts.text_segment.empty()) {
			cmd << " -Wl,-Ttext-segment=" << opts.text_segment;
		}

		// Add extra flags
		for (const auto& flag : opts.extra_flags) {
			cmd << " " << flag;
		}

		cmd << " " << source_path;
		cmd << " -o " << output_path;
		cmd << " 2>&1";  // Capture stderr

		// Execute compiler
		std::string command = cmd.str();
		std::string output = exec(command);

		// Check if compilation succeeded
		if (!std::filesystem::exists(output_path)) {
			throw std::runtime_error(
				"C++ compilation failed for " + name + ":\n" +
				"Command: " + command + "\n" +
				"Output: " + output);
		}

		// Read the compiled binary
		return read_binary(output_path);
	}

	const std::string& compiler() const { return m_compiler; }

private:
	std::string m_compiler;

	bool find_program(const std::string& name) {
		std::string cmd = "which " + name + " > /dev/null 2>&1";
		return system(cmd.c_str()) == 0;
	}

	std::string exec(const std::string& cmd) {
		std::array<char, 128> buffer;
		std::string result;

		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe) {
			throw std::runtime_error("popen() failed");
		}

		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			result += buffer.data();
		}

		pclose(pipe);
		return result;
	}

	std::vector<uint8_t> read_binary(const std::string& path) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file) {
			throw std::runtime_error("Failed to open compiled binary: " + path);
		}

		auto size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			throw std::runtime_error("Failed to read compiled binary: " + path);
		}

		return buffer;
	}
};

} // namespace loongarch::test
