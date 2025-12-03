#pragma once

#include <libloong/machine.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <stdexcept>
#include <tuple>

namespace loongarch::script {

// Forward declarations
class ScriptException;

// Helper: Extract function signature from callable
template<typename>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<R(Args...)> {
	using return_type = R;
	using args_tuple = std::tuple<Args...>;
};

template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> : function_traits<R(Args...)> {};

template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {};

template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {};

template<typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

// Helper to strip Machine& from front of argument list
template<typename Tuple>
struct strip_machine_arg;

template<typename First, typename... Rest>
struct strip_machine_arg<std::tuple<First, Rest...>> {
	using type = std::tuple<Rest...>;
};

template<>
struct strip_machine_arg<std::tuple<>> {
	using type = std::tuple<>;
};

// Helper to call function with arguments extracted from machine
template<typename Ret, typename F, typename... Args>
static void call_with_args(Machine& machine, F& callback, std::tuple<Args...>) {
	if constexpr (sizeof...(Args) == 0) {
		// No arguments beyond Machine&
		if constexpr (std::is_void_v<Ret>) {
			callback(machine);
		} else {
			Ret result = callback(machine);
			machine.set_result(result);
		}
	} else {
		// Extract arguments from registers (excluding Machine&)
		auto args = machine.template sysargs<Args...>();
		if constexpr (std::is_void_v<Ret>) {
			std::apply([&callback, &machine](auto&&... args) {
				callback(machine, std::forward<decltype(args)>(args)...);
			}, args);
		} else {
			Ret result = std::apply([&callback, &machine](auto&&... args) {
				return callback(machine, std::forward<decltype(args)>(args)...);
			}, args);
			machine.set_result(result);
		}
	}
}

// Helper to parse function name from signature
// e.g., "int host_add(int a, int b)" -> "host_add"
inline std::string parse_function_name(const std::string& signature) {
	// Find the opening parenthesis
	size_t paren_pos = signature.find('(');
	if (paren_pos == std::string::npos) {
		throw std::runtime_error("Invalid function signature: missing '(' in '" + signature + "'");
	}

	// Search backwards from the paren to find the function name
	size_t name_end = paren_pos;
	while (name_end > 0 && std::isspace(signature[name_end - 1])) {
		--name_end;
	}

	size_t name_start = name_end;
	while (name_start > 0 && (std::isalnum(signature[name_start - 1]) || signature[name_start - 1] == '_')) {
		--name_start;
	}

	if (name_start >= name_end) {
		throw std::runtime_error("Invalid function signature: no function name found in '" + signature + "'");
	}

	return signature.substr(name_start, name_end - name_start);
}

// Host binding entry
struct HostBinding {
	std::string name;
	std::string signature;  // e.g., "int add(int, int)"
	unsigned syscall_num;
};

// Static registry for host function bindings
class HostBindings {
public:
	// Register a host function (static, shared across all Script instances)
	template<typename F>
	static void register_function(const std::string& signature, F&& callback) {
		auto& registry = get_registry();

		// Parse function name from signature
		std::string name = parse_function_name(signature);

		// Check if already registered
		if (registry.bindings.find(name) != registry.bindings.end()) {
			throw std::runtime_error("Host function '" + name + "' already registered");
		}

		// Allocate syscall number
		if (registry.next_syscall > SYSCALL_MAX) {
			throw std::runtime_error("Too many host functions registered");
		}
		const int syscall_num = registry.next_syscall++;

		// Convert callable to std::function
		using traits = function_traits<std::remove_reference_t<F>>;
		using return_type = typename traits::return_type;
		using full_args_tuple = typename traits::args_tuple;
		// Strip Machine& from the front of the argument list
		using args_tuple = typename strip_machine_arg<full_args_tuple>::type;

		// Create wrapper that extracts arguments and calls callback
		auto wrapper = [callback = std::forward<F>(callback)](Machine& machine) mutable {
			call_with_args<return_type>(machine, callback, args_tuple{});
		};

		// Store binding
		HostBinding binding;
		binding.name = name;
		binding.signature = signature;
		binding.syscall_num = syscall_num;

		registry.bindings[name] = binding;

		// Store handler in vector for fast dispatch
		const unsigned idx = syscall_num - SYSCALL_BASE;
		if (registry.handlers.size() <= idx) {
			registry.handlers.resize(idx + 1);
		}
		registry.handlers[idx] = wrapper;
	}

	// Append user-defined header content
	static void append_header_content(const std::string& content) {
		auto& registry = get_registry();
		registry.header += content + "\n";
	}

	// Get user-defined header content
	static const std::string& get_header() {
		auto& registry = get_registry();
		return registry.header;
	}

	// Get all registered bindings
	static const std::unordered_map<std::string, HostBinding>& get_bindings() {
		return get_registry().bindings;
	}

	// Get binding by name
	static const HostBinding* get_binding(const std::string& name) {
		auto& bindings = get_registry().bindings;
		auto it = bindings.find(name);
		return (it != bindings.end()) ? &it->second : nullptr;
	}

	// Generate extern "C" declarations for all registered functions
	static std::string generate_extern_declarations() {
		std::string decls;
		for (const auto& [name, binding] : get_registry().bindings) {
			decls += "    " + binding.signature + ";\n";
		}
		return decls;
	}

	// Generate assembly stubs for all registered functions
	static std::string generate_asm_stubs() {
		std::string stubs;
		for (const auto& [name, binding] : get_registry().bindings) {
			stubs += "asm(\".pushsection .text\\n\"\n";
			stubs += "    \".global " + name + "\\n\"\n";
			stubs += "    \".type " + name + ", @function\\n\"\n";
			stubs += "    \"" + name + ":\\n\"\n";
			stubs += "    \"  ret\\n\"\n";
			stubs += "    \".popsection\\n\");\n";
		}
		return stubs;
	}

	// Dispatch a syscall to the appropriate host function
	static void dispatch(Machine& machine, unsigned syscall_num) {
		auto& registry = get_registry();

		// Direct vector lookup (syscall_num is sequential starting from SYSCALL_BASE)
		const unsigned idx = syscall_num - SYSCALL_BASE;
		if (idx < registry.handlers.size() && registry.handlers[idx]) {
			registry.handlers[idx](machine);
			return;
		}

		throw MachineException(ILLEGAL_OPERATION, "Unknown host callback", syscall_num);
	}

	// Clear all bindings (useful for testing)
	static void clear() {
		auto& registry = get_registry();
		registry.bindings.clear();
		registry.handlers.clear();
		registry.next_syscall = SYSCALL_BASE;
	}

private:
	static constexpr unsigned SYSCALL_BASE = 1024;
	static constexpr unsigned SYSCALL_MAX = 2047;

	struct Registry {
		std::unordered_map<std::string, HostBinding> bindings;
		std::vector<std::function<void(Machine&)>> handlers; // Direct syscall lookup vector
		unsigned next_syscall = SYSCALL_BASE;
		std::string header; // User-defined header content
	};

	// Singleton registry
	static inline Registry& get_registry() {
		static Registry registry;
		return registry;
	}
};

// Convenience macro for registering host functions
// Note: name parameter kept for generating unique struct names
#define REGISTER_HOST_FUNCTION(name, signature, callback) \
	namespace { \
		struct HostBinding_##name { \
			HostBinding_##name() { \
				loongarch::script::HostBindings::register_function(signature, callback); \
			} \
		}; \
		static HostBinding_##name host_binding_##name##_instance; \
	}

} // loongarch::script
