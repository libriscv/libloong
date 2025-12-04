#pragma once

namespace loongarch
{
	// Inline machine methods

	inline bool Machine::simulate(uint64_t max_instructions, uint64_t counter)
	{
		return cpu.simulate(cpu.pc(), counter, max_instructions);
	}

	inline void Machine::system_call(unsigned syscall_number)
	{
		if (syscall_number < m_syscall_handlers.size()) {
			auto* handler = m_syscall_handlers[syscall_number];
			handler(*this);
			return;
		}
		// Call unknown syscall handler if registered
		m_unknown_syscall_handler(*this, syscall_number);
	}

	inline void Machine::unchecked_system_call(unsigned syscall_number)
	{
		m_syscall_handlers[syscall_number](*this);
	}

	inline void Machine::install_syscall_handler(unsigned syscall_number, syscall_t* handler)
	{
		if (syscall_number < m_syscall_handlers.size()) {
			m_syscall_handlers[syscall_number] = handler;
		}
	}

	inline void Machine::set_unknown_syscall_handler(unknown_syscall_t* handler)
	{
		m_unknown_syscall_handler = handler;
	}

	template <typename T>
	inline void Machine::set_result(const T& value)
	{
		if constexpr (std::is_integral_v<remove_cvref_t<T>>) {
			if constexpr (sizeof(T) < sizeof(address_t) && !std::is_same_v<T, bool>)
				// Sign-extend all arguments smaller than the word size
				cpu.reg(REG_A0) = static_cast<address_t>(static_cast<std::make_signed_t<T>>(value));
			else
				cpu.reg(REG_A0) = static_cast<address_t>(value);
		} else if constexpr (std::is_floating_point_v<remove_cvref_t<T>>) {
			if constexpr (sizeof(T) == 4) {
				cpu.registers().getfl32(REG_FA0) = static_cast<float>(value);
			} else if constexpr (sizeof(T) == 8) {
				cpu.registers().getfl64(REG_FA0) = static_cast<double>(value);
			} else {
				static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point size");
			}
		} else {
			static_assert(sizeof(T) <= 2 * sizeof(address_t), "Result type too large");
		}
	}

	template <typename T>
	inline T Machine::return_value() const
	{
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
			"Return value type must be integral or floating point");
		if constexpr (std::is_integral_v<T>) {
			return static_cast<T>(cpu.reg(REG_A0));
		} else if constexpr (std::is_floating_point_v<T>) {
			if constexpr (sizeof(T) == 4) {
				return static_cast<T>(cpu.registers().getfl32(REG_FA0));
			} else if constexpr (sizeof(T) == 8) {
				return static_cast<T>(cpu.registers().getfl64(REG_FA0));
			} else {
				static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported floating point size");
			}
		}
		return T{};
	}

	template <typename T>
	inline T Machine::sysarg(int idx) const
	{
		if constexpr (std::is_pointer_v<remove_cvref_t<T>>) {
			return (T)memory.template memarray<std::remove_pointer_t<std::remove_reference_t<T>>>(cpu.reg(REG_A0 + idx), 1);
		}
		else if constexpr (std::is_integral_v<T> && !std::is_enum_v<T>)
			return static_cast<T>(cpu.reg(REG_A0 + idx));
		else if constexpr (std::is_same_v<T, float>)
			return cpu.registers().getfl32(REG_FA0 + idx);
		else if constexpr (std::is_same_v<T, double>)
			return cpu.registers().getfl64(REG_FA0 + idx);
		else if constexpr (std::is_enum_v<T>)
			return static_cast<T>(cpu.reg(REG_A0 + idx));
		else if constexpr (std::is_same_v<T, std::basic_string_view<char>>)
			return memory.memview(
				cpu.reg(REG_A0 + idx), cpu.reg(REG_A0 + idx + 1));
		else if constexpr (is_stdstring<T>::value)
			return memory.memstring(cpu.reg(REG_A0 + idx));
		else if constexpr (std::is_standard_layout_v<remove_cvref_t<T>> && std::is_trivial_v<remove_cvref_t<T>>) {
			T value;
			memory.copy_from_guest(&value, cpu.reg(REG_A0 + idx), sizeof(T));
			return value;
		} else
			static_assert(always_false<T>, "Unknown type");
	}

	template<typename... Args, std::size_t... Indices>
	inline auto Machine::resolve_args(std::index_sequence<Indices...>) const
	{
		std::tuple<std::decay_t<Args>...> retval;
		size_t i = 0;
		size_t f = 0;
		([&] {
			if constexpr (std::is_pointer_v<remove_cvref_t<Args>>)
				std::get<Indices>(retval) = sysarg<Args>(i++);
			else if constexpr (std::is_integral_v<Args>) {
				std::get<Indices>(retval) = sysarg<Args>(i++);
				if constexpr (sizeof(Args) > sizeof(address_t)) i++; // uses 2 registers
			}
			else if constexpr (std::is_floating_point_v<Args>)
				std::get<Indices>(retval) = sysarg<Args>(f++);
			else if constexpr (std::is_enum_v<Args>)
				std::get<Indices>(retval) = sysarg<Args>(i++);
			else if constexpr (std::is_same_v<Args, std::basic_string_view<char>>) {
				std::get<Indices>(retval) = sysarg<Args>(i); i+= 2;
			}
			else if constexpr (is_stdstring<Args>::value)
				std::get<Indices>(retval) = sysarg<Args>(i++);
			else if constexpr (std::is_standard_layout_v<remove_cvref_t<Args>> && std::is_trivial_v<remove_cvref_t<Args>>)
				std::get<Indices>(retval) = sysarg<Args>(i++);
			else
				static_assert(always_false<Args>, "Unknown type");
		}(), ...);
		return retval;
	}

	template<typename... Args>
	inline auto Machine::sysargs() const {
		return resolve_args<Args...>(std::index_sequence_for<Args...>{});
	}

} // loongarch
