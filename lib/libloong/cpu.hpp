#pragma once
#include "common.hpp"
#include "registers.hpp"
#include "la_instr.hpp"
#include "decoded_exec_segment.hpp"
#include <functional>
#include <memory>

namespace loongarch
{
	struct Machine;
	struct Memory;

	struct Instruction {
		using handler_t = void(*)(CPU&, la_instruction);
		using printer_t = int(*)(char*, size_t, const CPU&, la_instruction, address_t);

		handler_t handler;
		printer_t printer;

		constexpr Instruction(handler_t h, printer_t p = nullptr)
			: handler(h), printer(p) {}
	};

	struct CPU
	{
		using format_t = la_instruction;
		using instruction_t = Instruction;
		using breakpoint_t = std::function<void(CPU&)>;

		CPU(Machine& machine);
		CPU(Machine& machine, const Machine& other);

		void reset();

		// Simulation methods
		bool simulate(address_t pc, uint64_t icounter, uint64_t maxcounter);
		void simulate_inaccurate(address_t pc);
		void simulate_precise();
		void step_one(bool use_instruction_counter = true);

		// Register access
		LA_ALWAYS_INLINE auto& registers() noexcept { return m_regs; }
		LA_ALWAYS_INLINE const auto& registers() const noexcept { return m_regs; }

		// PC operations
		address_t pc() const noexcept { return m_regs.pc; }
		void jump(address_t addr);
		void aligned_jump(address_t addr) noexcept { m_regs.pc = addr; }
		void increment_pc(int delta) noexcept { m_regs.pc += delta; }

		// Register shortcuts
		auto& reg(uint32_t idx) noexcept {
			return m_regs.get(idx);
		}
		const auto& reg(uint32_t idx) const noexcept {
			return m_regs.get(idx);
		}

		// LL/SC support
		bool ll_bit() const noexcept { return m_ll_bit; }
		void set_ll_bit(bool value) noexcept { m_ll_bit = value; }

		// Machine access
		Machine& machine() noexcept;
		const Machine& machine() const noexcept;

		Memory& memory() noexcept;
		const Memory& memory() const noexcept;

		// Instruction execution
		void execute(format_t instr);
		static const instruction_t& decode(format_t instr);
		format_t read_current_instruction() const;
		void init_slowpath_execute_area(const void* data, address_t begin, address_t length);

		// Execute segments
		DecodedExecuteSegment& init_execute_area(const void* data, address_t begin, address_t length);
		void set_execute_segment(DecodedExecuteSegment& seg) noexcept { m_exec = &seg; }
		auto& current_execute_segment() noexcept { return *m_exec; }
		auto& current_execute_segment() const noexcept { return *m_exec; }

		// next_execute_segment() never fails; it throws on error
		struct NextExecuteReturn {
			DecodedExecuteSegment* exec;
			address_t pc;
		};
		NextExecuteReturn next_execute_segment(address_t pc);

		static std::shared_ptr<DecodedExecuteSegment>& empty_execute_segment() noexcept;
		bool is_executable(address_t addr) const noexcept;

		// Exception handling
		[[noreturn]]
		static void trigger_exception(ExceptionType type, address_t data = 0);

		// Debug support
		std::string to_string(format_t format) const;
		std::string current_instruction_to_string() const;

		// Debugging
		uint32_t install_ebreak_at(address_t addr);

		static const instruction_t& get_invalid_instruction() noexcept;
		static const instruction_t& get_unimplemented_instruction() noexcept;

	private:
		Registers m_regs;
		Machine& m_machine;
		DecodedExecuteSegment* m_exec;
		bool m_ll_bit = false; // LL/SC linked-load bit
	};

} // namespace loongarch

#include "cpu_inline.hpp"
