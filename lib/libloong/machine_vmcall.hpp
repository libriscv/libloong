#pragma once
#include "machine.hpp"
#include "guest_datatypes.hpp"
#include <string>
#include <vector>
#include <type_traits>
#include <cstring>

namespace loongarch
{
	// Type trait to check if a type is a standard layout type
	template <typename T>
	struct is_stdlayout : std::integral_constant<bool, std::is_standard_layout<T>::value> {};

	// Stack push implementation - pushes raw data onto the guest stack
	inline address_t Machine::stack_push(address_t& sp, const void* data, size_t size)
	{
		const size_t aligned_size = (size + 15) & ~size_t(15);
		sp -= aligned_size;
		memory.copy_to_guest(sp, data, size);
		return sp;
	}

	// Stack push implementation - pushes typed data onto the guest stack
	template <typename T>
	inline address_t Machine::stack_push(address_t& sp, const T& value)
	{
		return stack_push(sp, &value, sizeof(T));
	}

	// Setup call arguments according to LoongArch calling convention
	template <typename... Args> constexpr
	inline void setup_call(Machine& machine, address_t exit_addr, Args&&... args)
	{		auto& cpu = machine.cpu;

		// Set return address to exit function
		cpu.reg(REG_RA) = exit_addr;

		// Reset stack pointer to initial position
		address_t sp = machine.memory.stack_address();

		// Initialize argument registers
		int iarg = REG_A0; // Integer argument registers: A0-A7
		int farg = 0;      // Floating-point argument registers: FA0-FA7

		// Process each argument
		// Note: We avoid complex operations in the fold expression to keep it constexpr-friendly
		([&] {
			using T = remove_cvref_t<Args>;

			if constexpr (std::is_same_v<T, std::string>) {
				// std::string: Push zero-terminated string onto stack, pass pointer as integer
				cpu.reg(iarg++) = machine.stack_push(sp, args.c_str(), args.length() + 1);
			}
			else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
				// C-string: Push zero-terminated string onto stack, pass pointer as integer
				cpu.reg(iarg++) = machine.stack_push(sp, args, std::strlen(args) + 1);
			}
			else if constexpr (std::is_same_v<T, GuestStdString>) {
				// GuestStdString: Adjust SSO pointer and push onto stack
				args.move(sp - sizeof(T)); // SSO-adjustment
				cpu.reg(iarg++) = machine.stack_push(sp, &args, sizeof(T));
			}
			else if constexpr (is_scoped_guest_object<T>::value) {
				// ScopedArenaObject: Pass the guest address directly
				cpu.reg(iarg++) = args.address();
			}
			else if constexpr (std::is_integral_v<T>) {
				// Integer types go to integer registers (A0-A7)
				cpu.reg(iarg++) = static_cast<address_t>(args);
			}
			else if constexpr (std::is_floating_point_v<T>) {
				// Floating-point types go to FP registers (FA0-FA7)
				if constexpr (sizeof(T) == 4) {
					cpu.registers().getfl32(farg++) = static_cast<float>(args);
				} else if constexpr (sizeof(T) == 8) {
					cpu.registers().getfl64(farg++) = static_cast<double>(args);
				}
			}
			else if constexpr (std::is_enum_v<T>) {
				// Enums are treated as their underlying integer type
				using UnderlyingType = std::underlying_type_t<T>;
				cpu.reg(iarg++) = static_cast<address_t>(static_cast<UnderlyingType>(args));
			}
			else if constexpr (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>) {
				// Standard layout structs: Push onto stack, pass pointer as integer
				const address_t struct_addr = machine.stack_push(sp, args);
				cpu.reg(iarg++) = struct_addr;
			}
			else {
				static_assert(sizeof(T) == 0, "Unsupported argument type for vmcall. Use integers, floats, enums, strings, or standard-layout structs.");
			}
		}(), ...);

		// Align stack pointer to 16-byte boundary
		sp &= ~address_t(0xF);
		cpu.reg(REG_SP) = sp;
	}

	// vmcall: Call a guest function at a specific address
	template <typename Ret, uint64_t MAX_INSTRUCTIONS, typename... Args>
	inline Ret Machine::vmcall(address_t func_addr, Args&&... args)
	{
		// Use the exit address set via memory.set_exit_address()
		const address_t exit_addr = memory.exit_address();

		// Setup the call with arguments
		setup_call(*this, exit_addr, std::forward<Args>(args)...);

		// Set PC to the function address
		cpu.registers().pc = func_addr;

		// Execute until the function returns and calls exit
		if constexpr (MAX_INSTRUCTIONS == UINT64_MAX) {
			cpu.simulate_inaccurate(func_addr);
		} else {
			this->simulate(MAX_INSTRUCTIONS, 0);
			if (this->instruction_limit_reached()) {
				throw MachineException(MACHINE_TIMEOUT,
					"vmcall: Instruction limit reached", func_addr);
			}
		}

		// If we're here, execution completed normally (via exit syscall)
		// Handle return value based on type
		if constexpr (std::is_same_v<Ret, float>) {
			// Single-precision float is returned in FA0
			return cpu.registers().getfl32(REG_FA0);
		} else if constexpr (std::is_same_v<Ret, double>) {
			// Double-precision float is returned in FA0
			return cpu.registers().getfl64(REG_FA0);
		} else if constexpr (std::is_integral_v<Ret> || std::is_same_v<Ret, address_t>) {
			// Integer types are returned in A0
			return static_cast<Ret>(cpu.reg(REG_A0));
		} else {
			// Default: return value from A0
			return static_cast<Ret>(cpu.reg(REG_A0));
		}
	}

	// vmcall: Call a guest function by symbol name
	template <typename Ret, uint64_t MAX_INSTRUCTIONS, typename... Args>
	inline Ret Machine::vmcall(const std::string& func_name, Args&&... args)
	{
		// Resolve the function address
		const address_t func_addr = this->address_of(func_name);

		if (func_addr == 0) {
			throw MachineException(INVALID_PROGRAM,
				"vmcall: Function not found", 0);
		}

		// Call the function at the resolved address
		return vmcall<Ret, MAX_INSTRUCTIONS>(func_addr, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline void Machine::timed_vmcall(address_t func_addr, uint64_t max_instructions, Args&&... args)
	{
		// Use the exit address set via memory.set_exit_address()
		const address_t exit_addr = memory.exit_address();

		// Setup the call with arguments
		setup_call(*this, exit_addr, std::forward<Args>(args)...);

		// Set PC to the function address
		cpu.registers().pc = func_addr;

		// Execute until the function returns and calls exit
		this->simulate(max_instructions, 0);
		if (this->instruction_limit_reached()) {
			throw MachineException(MACHINE_TIMEOUT,
				"timed_vmcall: Instruction limit reached", func_addr);
		}
	}

	// preempt: Call a guest function with instruction limit and optional register save
	template <bool Throw, bool StoreRegs, typename... Args>
	inline address_t Machine::preempt(uint64_t max_instr, address_t func_addr, Args&&... args)
	{
		// Use the exit address set via memory.set_exit_address()
		const address_t exit_addr = memory.exit_address();

		// Optionally save CPU registers
		Registers saved_regs;
		if constexpr (StoreRegs) {
			saved_regs = cpu.registers();
		}

		// Allocate 16 bytes on stack for execution context
		cpu.reg(REG_SP) -= 16;

		// Setup the call
		setup_call(*this, exit_addr, std::forward<Args>(args)...);
		cpu.registers().pc = func_addr;

		// Execute with instruction limit
		this->simulate(max_instr, 0);

		// Restore registers if needed
		if constexpr (StoreRegs) {
			// Keep return value in A0
			const address_t retval = cpu.reg(REG_A0);
			cpu.registers() = saved_regs;
			cpu.reg(REG_A0) = retval;
		}

		if constexpr (Throw) {
			if (this->instruction_limit_reached()) {
				throw MachineException(MACHINE_TIMEOUT,
					"preempt: Instruction limit reached", func_addr);
			}
		}

		return cpu.reg(REG_A0);
	}

	// preempt: Call by symbol name with instruction limit
	template <bool Throw, bool StoreRegs, typename... Args>
	inline address_t Machine::preempt(uint64_t max_instr, const std::string& func_name, Args&&... args)
	{
		// Resolve the function address
		const address_t func_addr = this->address_of(func_name);

		if (func_addr == 0) {
			throw MachineException(INVALID_PROGRAM,
				"preempt: Function not found", 0);
		}

		// Call the function at the resolved address
		return preempt<Throw, StoreRegs>(max_instr, func_addr, std::forward<Args>(args)...);
	}

} // namespace loongarch
