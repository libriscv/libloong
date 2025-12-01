#pragma once
#include <libloong/machine.hpp>
#include <vector>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace loongarch::test {

// Helper to execute a sequence of raw instructions in isolation
class InstructionTester {
public:
	InstructionTester(uint64_t memory_size = 16ull << 20)
		: m_memory_size(memory_size)
	{
		// Create machine with custom arena (no ELF loading)
		MachineOptions options;
		options.verbose_loader = false;
		options.verbose_syscalls = false;
		options.memory_max = memory_size;

		m_machine = std::make_unique<Machine>(std::string_view{}, options);
		m_machine->set_max_instructions(1'000'000ull);
		// 64KB rodata starts at 0x10000, writable data at 0x20000 to end of arena
		m_machine->memory.allocate_custom_arena(memory_size, 0x10000, 0x20000);

		// Set up a default stack pointer (1MB to 128KB downwards)
		m_machine->memory.set_stack_address(0x100000);
		m_machine->cpu.reg(REG_SP) = m_machine->memory.stack_address();

		// Install minimal syscalls (just exit)
		m_machine->install_syscall_handler(SYSCALL_EXIT, [](Machine& m) {
			m.stop();
		});

		// Track next allocation address (start from data_start + some offset)
		m_next_alloc_addr = 0x100000; // Allocations from 1MB ...
	}

	// Execute a single instruction
	struct SingleInstructionResult {
		bool success = false;
		uint64_t pc_before = 0;
		uint64_t pc_after = 0;
		std::string error;
		uint64_t instructions_executed = 0;
	};

	SingleInstructionResult execute_one(uint32_t instruction, uint64_t pc = 0x10000) {
		SingleInstructionResult result;

		try {
			// Copy instructions into executable area and set PC
			m_machine->cpu.init_slowpath_execute_area(&instruction, pc, sizeof(uint32_t));

			result.pc_before = m_machine->cpu.pc();
			// Execute exactly one instruction
			uint64_t before_counter = m_machine->instruction_counter();
			m_machine->cpu.step_one(true);
			result.instructions_executed = m_machine->instruction_counter() - before_counter;

			result.pc_after = m_machine->cpu.pc();
			result.success = true;

		} catch (const MachineException& e) {
			fprintf(stderr, "Caught MachineException: %s\nRegisters:\n%s\n",
			        e.what(),
			        m_machine->cpu.registers().to_string().c_str());
			result.error = std::string("MachineException: ") + e.what() +
			               " (type=" + std::to_string(static_cast<int>(e.type())) + ")";
			result.pc_after = m_machine->cpu.pc();
		} catch (const std::exception& e) {
			result.error = std::string("Exception: ") + e.what();
			result.pc_after = m_machine->cpu.pc();
		}

		return result;
	}

	// Execute a sequence of instructions
	struct SequenceResult {
		bool success = false;
		uint64_t pc_before = 0;
		uint64_t pc_after = 0;
		std::string error;
		uint64_t instructions_executed = 0;
		std::vector<uint64_t> pc_trace; // PC after each instruction
	};

	SequenceResult execute_sequence(const std::vector<uint32_t>& instructions,
	                                 uint64_t pc = 0x10000,
	                                 bool trace_pc = false) {
		SequenceResult result;

		try {
			// Create executable area with all instructions
			m_machine->cpu.init_slowpath_execute_area(
				instructions.data(),
				pc,
				instructions.size() * sizeof(uint32_t)
			);

			result.pc_before = m_machine->cpu.pc();
			uint64_t before_counter = m_machine->instruction_counter();

			// Execute each instruction one by one
			for (size_t i = 0; i < instructions.size(); i++) {
				m_machine->cpu.step_one(true);

				if (trace_pc) {
					result.pc_trace.push_back(m_machine->cpu.pc());
				}
			}

			result.instructions_executed = m_machine->instruction_counter() - before_counter;
			result.pc_after = m_machine->cpu.pc();
			result.success = true;

		} catch (const MachineException& e) {
			fprintf(stderr, "Caught MachineException: %s\nRegisters:\n%s\n",
			        e.what(),
			        m_machine->cpu.registers().to_string().c_str());
			result.error = std::string("MachineException: ") + e.what() +
			               " (type=" + std::to_string(static_cast<int>(e.type())) + ")";
			result.pc_after = m_machine->cpu.pc();
		} catch (const std::exception& e) {
			result.error = std::string("Exception: ") + e.what();
			result.pc_after = m_machine->cpu.pc();
		}

		return result;
	}

	// Allocate guest memory and return the guest address
	uint64_t allocate_guest_memory(size_t size, size_t alignment = 32) {
		// Align the current allocation address
		m_next_alloc_addr = (m_next_alloc_addr + alignment - 1) & ~(alignment - 1);

		uint64_t addr = m_next_alloc_addr;

		// Round up size to alignment for next allocation
		size = (size + alignment - 1) & ~(alignment - 1);
		m_next_alloc_addr += size;

		// Check if we're still within arena bounds
		if (m_next_alloc_addr >= m_memory_size) {
			return 0; // Out of memory
		}

		// Zero the allocated memory
		m_machine->memory.memset(addr, 0, size);

		return addr;
	}

	// Read guest memory
	template <typename T>
	T read(uint64_t addr) {
		return m_machine->memory.template read<T>(addr);
	}

	// Write guest memory
	template <typename T>
	void write(uint64_t addr, T value) {
		m_machine->memory.template write<T>(addr, value);
	}

	// Read array from guest memory
	template <typename T>
	std::vector<T> read_array(uint64_t addr, size_t count) {
		const T* ptr = m_machine->memory.template memarray<T>(addr, count);
		return std::vector<T>(ptr, ptr + count);
	}

	// Write array to guest memory
	template <typename T>
	void write_array(uint64_t addr, const std::vector<T>& data) {
		T* ptr = m_machine->memory.template writable_memarray<T>(addr, data.size());
		std::memcpy(ptr, data.data(), data.size() * sizeof(T));
	}

	// Register access
	uint64_t get_reg(int reg) const {
		return m_machine->cpu.reg(reg);
	}
	void set_reg(int reg, uint64_t value) {
		m_machine->cpu.reg(reg) = value;
	}

	// FP register access
	double get_freg64(int reg) const {
		return m_machine->cpu.registers().getfl64(reg);
	}
	void set_freg64(int reg, double value) {
		m_machine->cpu.registers().getfl64(reg) = value;
	}

	float get_freg32(int reg) const {
		return m_machine->cpu.registers().getfl32(reg);
	}
	void set_freg32(int reg, float value) {
		m_machine->cpu.registers().getfl32(reg) = value;
	}

	// Vector register access (LSX/LASX)
	template <typename T>
	std::vector<T> get_vreg(int reg) const {
		constexpr size_t count = 16 / sizeof(T); // LSX is 128-bit (low part of 256-bit)
		std::vector<T> result(count);
		auto& vreg = m_machine->cpu.registers().getvr(reg);
		std::memcpy(result.data(), &vreg, 16);
		return result;
	}

	template <typename T>
	void set_vreg(int reg, const std::vector<T>& values) {
		auto& vreg = m_machine->cpu.registers().getvr(reg);
		std::memcpy(&vreg, values.data(),
		            std::min(values.size() * sizeof(T), size_t(16)));
	}

	// Extended vector register access (LASX - 256-bit)
	template <typename T>
	std::vector<T> get_xvreg(int reg) const {
		constexpr size_t count = 32 / sizeof(T); // LASX is 256-bit
		std::vector<T> result(count);
		auto& vreg = m_machine->cpu.registers().getvr(reg);
		std::memcpy(result.data(), &vreg, 32);
		return result;
	}

	template <typename T>
	void set_xvreg(int reg, const std::vector<T>& values) {
		auto& vreg = m_machine->cpu.registers().getvr(reg);
		std::memcpy(&vreg, values.data(),
		            std::min(values.size() * sizeof(T), size_t(32)));
	}

	// FCC (floating-point condition code) register access
	uint8_t get_fcc(int index) const {
		return m_machine->cpu.registers().cf(index);
	}

	void set_fcc(int index, uint8_t value) {
		m_machine->cpu.registers().set_cf(index, value);
	}

	// Reset machine state
	void reset() {
		m_machine->cpu.reset();
		m_machine->cpu.reg(REG_SP) = 0x800000;
		m_machine->set_instruction_counter(0);
	}

	// Direct machine access
	Machine& machine() { return *m_machine; }
	const Machine& machine() const { return *m_machine; }

	// Helper: Print register state (for debugging)
	std::string dump_registers() const {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer),
		         "  PC   0x%016lx\n", long(m_machine->cpu.pc()));
		const size_t offset = strlen(buffer);
		snprintf(buffer + offset, sizeof(buffer) - offset,
		         "%s\n", m_machine->cpu.registers().to_string().c_str());
		return std::string(buffer);
	}

	// Helper: Print FP registers
	std::string dump_fp_registers() const {
		std::ostringstream oss;
		oss << "Floating Point Registers:\n";
		for (int i = 0; i < 32; i++) {
			oss << "  f" << std::dec << i << " = "
			    << std::scientific << get_freg64(i) << "\n";
		}
		return oss.str();
	}

	// Helper: Print vector register as doubles
	std::string dump_xvreg_d(int reg) const {
		auto values = get_xvreg<double>(reg);
		std::ostringstream oss;
		oss << "xr" << reg << " (doubles): [";
		for (size_t i = 0; i < values.size(); i++) {
			if (i > 0) oss << ", ";
			oss << std::scientific << values[i];
		}
		oss << "]\n";
		return oss.str();
	}

private:
	static constexpr int SYSCALL_EXIT = 93;

	std::unique_ptr<Machine> m_machine;
	uint64_t m_memory_size;
	uint64_t m_next_alloc_addr; // Next address for guest memory allocation
};

} // namespace loongarch::test
