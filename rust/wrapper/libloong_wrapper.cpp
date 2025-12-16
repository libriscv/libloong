#include "libloong_wrapper.h"
#include <libloong/machine.hpp>
#include <cstring>
#include <exception>
#include <vector>

using namespace loongarch;

// Helper to catch C++ exceptions and convert to error codes
template<typename F>
LibLoongError safe_call(F&& func, LibLoongError default_error = LIBLOONG_ERROR_UNKNOWN) {
    try {
        func();
        return LIBLOONG_OK;
    } catch (const MachineTimeoutException&) {
        return LIBLOONG_ERROR_TIMEOUT;
    } catch (const MachineException&) {
        return LIBLOONG_ERROR_EXECUTION;
    } catch (const std::bad_alloc&) {
        return LIBLOONG_ERROR_OUT_OF_MEMORY;
    } catch (...) {
        return default_error;
    }
}

// Convert our options to libloong's MachineOptions
static MachineOptions to_machine_options(const LibLoongMachineOptions* opts) {
    MachineOptions result;
    if (opts) {
        result.memory_max = opts->memory_max;
        result.stack_size = opts->stack_size;
        result.brk_size = opts->brk_size;
        result.verbose_loader = opts->verbose_loader;
        result.verbose_syscalls = opts->verbose_syscalls;
        result.use_shared_execute_segments = opts->use_shared_execute_segments;
    }
    return result;
}

extern "C" {

LibLoongMachine* libloong_machine_create(
    const uint8_t* binary_data,
    size_t binary_size,
    const LibLoongMachineOptions* options,
    LibLoongError* error)
{
    if (!binary_data || binary_size == 0) {
        if (error) *error = LIBLOONG_ERROR_INVALID_ELF;
        return nullptr;
    }

    try {
        std::vector<uint8_t> binary(binary_data, binary_data + binary_size);
        MachineOptions opts = to_machine_options(options);

        Machine* machine = new Machine(binary, opts);
        if (error) *error = LIBLOONG_OK;
        return reinterpret_cast<LibLoongMachine*>(machine);
    } catch (const std::bad_alloc&) {
        if (error) *error = LIBLOONG_ERROR_OUT_OF_MEMORY;
        return nullptr;
    } catch (...) {
        if (error) *error = LIBLOONG_ERROR_INVALID_ELF;
        return nullptr;
    }
}

void libloong_machine_destroy(LibLoongMachine* machine) {
    if (machine) {
        delete reinterpret_cast<Machine*>(machine);
    }
}

LibLoongError libloong_machine_setup_linux(
    LibLoongMachine* machine,
    const char** args,
    size_t argc,
    const char** env,
    size_t envc)
{
    if (!machine) return LIBLOONG_ERROR_EXECUTION;

    Machine* m = reinterpret_cast<Machine*>(machine);

    std::vector<std::string> args_vec;
    for (size_t i = 0; i < argc; i++) {
        args_vec.emplace_back(args[i]);
    }

    std::vector<std::string> env_vec;
    for (size_t i = 0; i < envc; i++) {
        env_vec.emplace_back(env[i]);
    }

    return safe_call([&]() {
        m->setup_linux(args_vec, env_vec);
    });
}

void libloong_machine_setup_minimal_syscalls(void) {
    Machine::setup_minimal_syscalls();
}

void libloong_machine_setup_linux_syscalls(void) {
    Machine::setup_linux_syscalls();
}

LibLoongError libloong_machine_setup_accelerated_syscalls(LibLoongMachine* machine) {
    if (!machine) return LIBLOONG_ERROR_EXECUTION;
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call([&]() {
        m->setup_accelerated_syscalls();
    });
}

LibLoongError libloong_machine_setup_accelerated_heap(
    LibLoongMachine* machine,
    uint64_t arena_base,
    size_t arena_size)
{
    if (!machine) return LIBLOONG_ERROR_EXECUTION;
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call([&]() {
        m->setup_accelerated_heap(arena_base, arena_size);
    });
}

LibLoongError libloong_machine_simulate(
    LibLoongMachine* machine,
    uint64_t max_instructions,
    uint64_t counter)
{
    if (!machine) return LIBLOONG_ERROR_EXECUTION;
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call([&]() {
        m->simulate(max_instructions, counter);
    });
}

void libloong_machine_stop(LibLoongMachine* machine) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->stop();
    }
}

int libloong_machine_stopped(const LibLoongMachine* machine) {
    return machine ? reinterpret_cast<const Machine*>(machine)->stopped() : 1;
}

int libloong_machine_instruction_limit_reached(const LibLoongMachine* machine) {
    return machine ? reinterpret_cast<const Machine*>(machine)->instruction_limit_reached() : 0;
}

uint64_t libloong_machine_instruction_counter(const LibLoongMachine* machine) {
    return machine ? reinterpret_cast<const Machine*>(machine)->instruction_counter() : 0;
}

void libloong_machine_set_instruction_counter(LibLoongMachine* machine, uint64_t val) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->set_instruction_counter(val);
    }
}

void libloong_machine_increment_counter(LibLoongMachine* machine, uint64_t val) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->increment_counter(val);
    }
}

uint64_t libloong_machine_max_instructions(const LibLoongMachine* machine) {
    return machine ? reinterpret_cast<const Machine*>(machine)->max_instructions() : 0;
}

void libloong_machine_set_max_instructions(LibLoongMachine* machine, uint64_t val) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->set_max_instructions(val);
    }
}

void libloong_install_syscall_handler(unsigned sysnum, LibLoongSyscallHandler handler) {
    if (handler) {
        Machine::install_syscall_handler(sysnum,
            reinterpret_cast<Machine::syscall_t*>(handler));
    }
}

void libloong_machine_system_call(LibLoongMachine* machine, unsigned sysnum) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->system_call(sysnum);
    }
}

uint64_t libloong_machine_sysarg(const LibLoongMachine* machine, int idx) {
    if (!machine) return 0;
    return reinterpret_cast<const Machine*>(machine)->sysarg(idx);
}

void libloong_machine_set_result(LibLoongMachine* machine, uint64_t value) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->set_result(value);
    }
}

uint64_t libloong_machine_return_value(const LibLoongMachine* machine) {
    if (!machine) return 0;
    return reinterpret_cast<const Machine*>(machine)->return_value();
}

LibLoongError libloong_machine_vmcall(
    LibLoongMachine* machine,
    uint64_t func_addr,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    uint64_t* return_value)
{
    if (!machine) return LIBLOONG_ERROR_EXECUTION;
    Machine* m = reinterpret_cast<Machine*>(machine);

    LibLoongError result = LIBLOONG_OK;

    try {
        // Call with up to 8 arguments
        uint64_t ret = 0;
        switch (arg_count) {
            case 0: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr); break;
            case 1: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0]); break;
            case 2: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1]); break;
            case 3: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1], args[2]); break;
            case 4: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1], args[2], args[3]); break;
            case 5: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1], args[2], args[3], args[4]); break;
            case 6: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1], args[2], args[3], args[4], args[5]); break;
            case 7: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1], args[2], args[3], args[4], args[5], args[6]); break;
            case 8: ret = m->vmcall<uint64_t, UINT64_MAX>(func_addr, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]); break;
            default: return LIBLOONG_ERROR_EXECUTION;
        }
        if (return_value) *return_value = ret;
    } catch (const MachineTimeoutException&) {
        result = LIBLOONG_ERROR_TIMEOUT;
    } catch (const MachineException&) {
        result = LIBLOONG_ERROR_EXECUTION;
    } catch (...) {
        result = LIBLOONG_ERROR_UNKNOWN;
    }

    return result;
}

LibLoongError libloong_machine_vmcall_by_name(
    LibLoongMachine* machine,
    const char* func_name,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    uint64_t* return_value)
{
    if (!machine || !func_name) return LIBLOONG_ERROR_EXECUTION;
    Machine* m = reinterpret_cast<Machine*>(machine);

    uint64_t addr = m->address_of(func_name);
    if (addr == 0) {
        return LIBLOONG_ERROR_SYMBOL_NOT_FOUND;
    }

    return libloong_machine_vmcall(machine, addr, max_instructions, args, arg_count, return_value);
}

uint64_t libloong_machine_address_of(const LibLoongMachine* machine, const char* name) {
    if (!machine || !name) return 0;
    return reinterpret_cast<const Machine*>(machine)->address_of(name);
}

int libloong_machine_has_symbol(const LibLoongMachine* machine, const char* name) {
    if (!machine || !name) return 0;
    return reinterpret_cast<const Machine*>(machine)->address_of(name) != 0;
}

LibLoongError libloong_machine_read_memory(
    const LibLoongMachine* machine,
    uint64_t addr,
    void* data,
    size_t size)
{
    if (!machine || !data) return LIBLOONG_ERROR_EXECUTION;
    const Machine* m = reinterpret_cast<const Machine*>(machine);

    return safe_call([&]() {
        m->memory.copy_from_guest(data, addr, size);
    }, LIBLOONG_ERROR_INVALID_ADDRESS);
}

LibLoongError libloong_machine_write_memory(
    LibLoongMachine* machine,
    uint64_t addr,
    const void* data,
    size_t size)
{
    if (!machine || !data) return LIBLOONG_ERROR_EXECUTION;
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call([&]() {
        m->memory.copy_to_guest(addr, data, size);
    }, LIBLOONG_ERROR_INVALID_ADDRESS);
}

LibLoongError libloong_machine_read_string(
    const LibLoongMachine* machine,
    uint64_t addr,
    char* buffer,
    size_t max_len,
    size_t* actual_len)
{
    if (!machine || !buffer || max_len == 0) return LIBLOONG_ERROR_EXECUTION;
    const Machine* m = reinterpret_cast<const Machine*>(machine);

    return safe_call([&]() {
        std::string str = m->memory.memstring(addr, max_len);
        size_t len = std::min(str.size(), max_len - 1);
        std::memcpy(buffer, str.data(), len);
        buffer[len] = '\0';
        if (actual_len) *actual_len = len;
    }, LIBLOONG_ERROR_INVALID_ADDRESS);
}

uint64_t libloong_machine_get_register(const LibLoongMachine* machine, unsigned reg_num) {
    if (!machine || reg_num >= 32) return 0;
    return reinterpret_cast<const Machine*>(machine)->cpu.reg(reg_num);
}

void libloong_machine_set_register(LibLoongMachine* machine, unsigned reg_num, uint64_t value) {
    if (machine && reg_num < 32) {
        reinterpret_cast<Machine*>(machine)->cpu.reg(reg_num) = value;
    }
}

uint64_t libloong_machine_get_pc(const LibLoongMachine* machine) {
    if (!machine) return 0;
    return reinterpret_cast<const Machine*>(machine)->cpu.pc();
}

void libloong_machine_set_pc(LibLoongMachine* machine, uint64_t pc) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->cpu.jump(pc);
    }
}

// Static callback storage
static LibLoongStdoutCallback g_stdout_callback = nullptr;

static void stdout_callback_wrapper(const char* data, size_t len) {
    if (g_stdout_callback) {
        g_stdout_callback(data, len);
    }
}

void libloong_machine_set_stdout_callback(LibLoongStdoutCallback callback) {
    g_stdout_callback = callback;
    if (callback) {
        Machine::set_print_callback(stdout_callback_wrapper);
    } else {
        Machine::set_print_callback(nullptr);
    }
}

void libloong_machine_set_userdata(LibLoongMachine* machine, void* userdata) {
    if (machine) {
        reinterpret_cast<Machine*>(machine)->set_userdata(userdata);
    }
}

void* libloong_machine_get_userdata(const LibLoongMachine* machine) {
    if (!machine) return nullptr;
    return reinterpret_cast<const Machine*>(machine)->get_userdata<void>();
}

const char* libloong_error_string(LibLoongError error) {
    switch (error) {
        case LIBLOONG_OK: return "Success";
        case LIBLOONG_ERROR_INVALID_ELF: return "Invalid ELF binary";
        case LIBLOONG_ERROR_EXECUTION: return "Execution error";
        case LIBLOONG_ERROR_TIMEOUT: return "Instruction limit exceeded";
        case LIBLOONG_ERROR_INVALID_ADDRESS: return "Invalid memory address";
        case LIBLOONG_ERROR_SYMBOL_NOT_FOUND: return "Symbol not found";
        case LIBLOONG_ERROR_OUT_OF_MEMORY: return "Out of memory";
        default: return "Unknown error";
    }
}

LibLoongMachineOptions libloong_default_options(void) {
    LibLoongMachineOptions opts;
    opts.memory_max = 256 * 1024 * 1024;  // 256 MB
    opts.stack_size = 2 * 1024 * 1024;     // 2 MB
    opts.brk_size = 1 * 1024 * 1024;       // 1 MB
    opts.verbose_loader = 0;
    opts.verbose_syscalls = 0;
    opts.use_shared_execute_segments = 1;
    return opts;
}

} // extern "C"
