#pragma once

#include "script.hpp"
#include <functional>
#include <unordered_map>
#include <memory>

namespace loongarch::script {

// Forward declaration
class Script;

// Syscall number range for host callbacks (400-511, avoiding conflicts)
static constexpr unsigned SYSCALL_HOST_BASE = 400;
static constexpr unsigned SYSCALL_HOST_MAX = 511;

// Helper: Extract function signature from std::function
template<typename>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Event class: Host -> Guest function calls
// Usage: Event<void(int, float)> my_event{script, "my_event"};
//        my_event(42, 3.14f);
template<typename F>
class Event {
public:
    using func_type = std::function<F>;
    using traits = function_traits<func_type>;
    using return_type = typename traits::return_type;

    Event(Script& script, const std::string& function_name)
        : script_(script)
        , function_name_(function_name)
        , func_addr_(0)
        , cached_(false)
    {
    }

    // Call operator - invokes the guest function
    template<typename... Args>
    return_type operator()(Args&&... args) {
        if (!cached_) {
            func_addr_ = script_.address_of(function_name_);
            cached_ = true;
        }

        if constexpr (std::is_void_v<return_type>) {
            script_.machine().template vmcall<0>(func_addr_, std::forward<Args>(args)...);
        } else {
            return script_.machine().template vmcall<0, return_type>(func_addr_, std::forward<Args>(args)...);
        }
    }

    // Check if the function exists in the guest
    bool exists() const {
        return script_.has_function(function_name_);
    }

    // Get the cached address
    address_t address() const {
        if (!cached_) {
            func_addr_ = script_.address_of(function_name_);
            cached_ = true;
        }
        return func_addr_;
    }

private:
    Script& script_;
    std::string function_name_;
    mutable address_t func_addr_;
    mutable bool cached_;
};

// Callback class: Guest -> Host function calls
// Usage: script.bind("my_host_func", [](int x) { return x * 2; });
class HostCallbackManager {
public:
    explicit HostCallbackManager(Script& script)
        : script_(script)
        , next_syscall_(SYSCALL_HOST_BASE)
    {
        // Install the unknown syscall handler to route callbacks
        Machine::set_unknown_syscall_handler(
            [](Machine& machine) {
                auto& self = *static_cast<HostCallbackManager*>(machine.get_userdata());
                self.dispatch_callback(machine);
            }
        );
    }

    // Bind a host function to a guest stub
    // The guest must have a function like: void my_host_func() {}
    template<typename Ret, typename... Args>
    void bind(const std::string& function_name, std::function<Ret(Args...)> callback) {
        if (next_syscall_ > SYSCALL_HOST_MAX) {
            throw ScriptException("Too many host callbacks registered");
        }

        unsigned syscall_num = next_syscall_++;

        // Store the callback wrapper
        auto wrapper = [callback](Machine& machine) {
            if constexpr (sizeof...(Args) == 0) {
                // No arguments
                if constexpr (std::is_void_v<Ret>) {
                    callback();
                } else {
                    Ret result = callback();
                    machine.set_result(result);
                }
            } else {
                // Extract arguments from registers
                auto args = machine.template sysargs<Args...>();
                if constexpr (std::is_void_v<Ret>) {
                    std::apply(callback, args);
                } else {
                    Ret result = std::apply(callback, args);
                    machine.set_result(result);
                }
            }
        };

        callbacks_[syscall_num] = wrapper;

        // Patch the guest function to invoke this syscall
        patch_function(function_name, syscall_num);
    }

    // Convenience overload for lambdas
    template<typename F>
    void bind(const std::string& function_name, F&& callback) {
        bind(function_name, make_function(std::forward<F>(callback)));
    }

private:
    // Convert lambda to std::function
    template<typename F>
    static auto make_function(F&& f) {
        return make_function_impl(&F::operator(), std::forward<F>(f));
    }

    template<typename R, typename C, typename... Args>
    static std::function<R(Args...)> make_function_impl(R (C::*)(Args...) const, C&& c) {
        return std::function<R(Args...)>(std::forward<C>(c));
    }

    template<typename R, typename C, typename... Args>
    static std::function<R(Args...)> make_function_impl(R (C::*)(Args...), C&& c) {
        return std::function<R(Args...)>(std::forward<C>(c));
    }

    // Patch a guest function to invoke a syscall
    void patch_function(const std::string& function_name, unsigned syscall_num) {
        auto addr = script_.address_of(function_name);

        // Get the decoder cache entry and patch it to SYSCALLIMM
        DecoderData entry;
        entry.bytecode = LA64_BC_SYSCALLIMM;
        entry.handler_idx = 0;
        entry.block_bytes = 0; // Diverges here
        entry.instr = syscall_num;

        // Install into decoder cache
        auto& exec_seg = *script_.machine().memory.exec_segment_for(addr);
        exec_seg.set(addr, entry);
    }

    // Dispatch unknown syscalls to registered callbacks
    void dispatch_callback(Machine& machine) {
        // The syscall number is in A7
        unsigned syscall_num = machine.cpu.reg(REG_A7);

        auto it = callbacks_.find(syscall_num);
        if (it != callbacks_.end()) {
            it->second(machine);
        } else {
            throw MachineException(ILLEGAL_OPERATION, "Unknown host callback", syscall_num);
        }
    }

    Script& script_;
    unsigned next_syscall_;
    std::unordered_map<unsigned, std::function<void(Machine&)>> callbacks_;
};

} // namespace loongarch::script
