#pragma once

namespace loongarch
{
	// Inline machine methods

	template <int W>
	template <bool Throw>
	inline bool Machine<W>::simulate(uint64_t max_instructions, uint64_t counter)
	{
		return cpu.simulate(cpu.pc(), counter, max_instructions);
	}

	template <int W>
	inline void Machine<W>::system_call(unsigned syscall_number)
	{
		if (syscall_number < m_syscall_handlers.size()) {
			auto* handler = m_syscall_handlers[syscall_number];
			handler(*this);
			return;
		}
		throw MachineException(ILLEGAL_OPERATION, "Unknown system call", syscall_number);
	}

	template <int W>
	inline void Machine<W>::install_syscall_handler(unsigned syscall_number, syscall_t* handler)
	{
		if (syscall_number < m_syscall_handlers.size()) {
			m_syscall_handlers[syscall_number] = handler;
		}
	}

	template <int W>
	template <typename T>
	inline void Machine<W>::set_result(const T& value)
	{
		if constexpr (std::is_integral_v<remove_cvref_t<T>>) {
			cpu.reg(REG_A0) = static_cast<address_t>(value);
		} else if constexpr (std::is_floating_point_v<remove_cvref_t<T>>) {
			if constexpr (sizeof(T) == 4) {
				cpu.reg_f(REG_A0).f32 = static_cast<float>(value);
			} else if constexpr (sizeof(T) == 8) {
				cpu.reg_f(REG_A0).f64 = static_cast<double>(value);
			} else {
				static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point size");
			}
		} else {
			static_assert(sizeof(T) <= 2 * sizeof(address_t), "Result type too large");
		}
	}

	template <int W>
	template <typename T>
	inline T Machine<W>::return_value() const
	{
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
			"Return value type must be integral or floating point");
		if constexpr (std::is_integral_v<T>) {
			return static_cast<T>(cpu.reg(REG_A0));
		} else if constexpr (std::is_floating_point_v<T>) {
			if constexpr (sizeof(T) == 4) {
				return static_cast<T>(cpu.reg_f(REG_A0).f32);
			} else if constexpr (sizeof(T) == 8) {
				return static_cast<T>(cpu.reg_f(REG_A0).f64);
			} else {
				static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point size");
			}
		}
		return T{};
	}

} // loongarch
