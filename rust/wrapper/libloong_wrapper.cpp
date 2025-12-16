#include "libloong_wrapper.h"
#include <libloong/machine.hpp>
#include <cstring>
#include <exception>
#include <vector>

using namespace loongarch;

// Helper to convert exception type to LibLoongExceptionType
static LibLoongExceptionType to_libloong_exception_type(ExceptionType type) {
    switch (type) {
        case ILLEGAL_OPCODE: return LIBLOONG_EXCEPTION_ILLEGAL_OPCODE;
        case ILLEGAL_OPERATION: return LIBLOONG_EXCEPTION_ILLEGAL_OPERATION;
        case PROTECTION_FAULT: return LIBLOONG_EXCEPTION_PROTECTION_FAULT;
        case EXECUTION_SPACE_PROTECTION_FAULT: return LIBLOONG_EXCEPTION_EXECUTION_SPACE_PROTECTION_FAULT;
        case MISALIGNED_INSTRUCTION: return LIBLOONG_EXCEPTION_MISALIGNED_INSTRUCTION;
        case UNIMPLEMENTED_INSTRUCTION: return LIBLOONG_EXCEPTION_UNIMPLEMENTED_INSTRUCTION;
        case MACHINE_TIMEOUT: return LIBLOONG_EXCEPTION_MACHINE_TIMEOUT;
        case OUT_OF_MEMORY: return LIBLOONG_EXCEPTION_OUT_OF_MEMORY;
        case INVALID_PROGRAM: return LIBLOONG_EXCEPTION_INVALID_PROGRAM;
        case FEATURE_DISABLED: return LIBLOONG_EXCEPTION_FEATURE_DISABLED;
        case UNIMPLEMENTED_SYSCALL: return LIBLOONG_EXCEPTION_UNIMPLEMENTED_SYSCALL;
        case GUEST_ABORT: return LIBLOONG_EXCEPTION_GUEST_ABORT;
        default: return LIBLOONG_EXCEPTION_NONE;
    }
}

// Helper to catch C++ exceptions and convert to error codes
template<typename F> static
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

// Helper to catch C++ exceptions and convert to extended error info
template<typename F> static
LibLoongError safe_call_ex(F&& func, LibLoongErrorInfo* error_info, LibLoongError default_error = LIBLOONG_ERROR_UNKNOWN) {
    try {
        func();
        if (error_info) {
            error_info->error_code = LIBLOONG_OK;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            error_info->message[0] = '\0';
        }
        return LIBLOONG_OK;
    } catch (const MachineTimeoutException& e) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_TIMEOUT;
            error_info->exception_type = LIBLOONG_EXCEPTION_MACHINE_TIMEOUT;
            error_info->data = e.data();
            std::strncpy(error_info->message, e.what(), sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return LIBLOONG_ERROR_TIMEOUT;
    } catch (const MachineException& e) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_EXECUTION;
            error_info->exception_type = to_libloong_exception_type(e.type());
            error_info->data = e.data();
            std::strncpy(error_info->message, e.what(), sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return LIBLOONG_ERROR_EXECUTION;
    } catch (const std::bad_alloc& e) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_OUT_OF_MEMORY;
            error_info->exception_type = LIBLOONG_EXCEPTION_OUT_OF_MEMORY;
            error_info->data = 0;
            std::strncpy(error_info->message, e.what(), sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return LIBLOONG_ERROR_OUT_OF_MEMORY;
    } catch (const std::exception& e) {
        if (error_info) {
            error_info->error_code = default_error;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            std::strncpy(error_info->message, e.what(), sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return default_error;
    } catch (...) {
        if (error_info) {
            error_info->error_code = default_error;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            std::strncpy(error_info->message, "Unknown exception", sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
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
    LibLoongErrorInfo* error_info)
{
    if (!binary_data || binary_size == 0) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_INVALID_ELF;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            std::strncpy(error_info->message, "Invalid binary data", sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return nullptr;
    }

    try {
        std::vector<uint8_t> binary(binary_data, binary_data + binary_size);
        MachineOptions opts = to_machine_options(options);

        Machine* machine = new Machine(binary, opts);
		auto exit_addr = machine->address_of("_exit");
		machine->memory.set_exit_address(exit_addr);
        if (error_info) {
            error_info->error_code = LIBLOONG_OK;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            error_info->message[0] = '\0';
        }
        return reinterpret_cast<LibLoongMachine*>(machine);
    } catch (const std::bad_alloc& e) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_OUT_OF_MEMORY;
            error_info->exception_type = LIBLOONG_EXCEPTION_OUT_OF_MEMORY;
            error_info->data = 0;
            std::strncpy(error_info->message, e.what(), sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return nullptr;
    } catch (const std::exception& e) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_INVALID_ELF;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            std::strncpy(error_info->message, e.what(), sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
        return nullptr;
    } catch (...) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_INVALID_ELF;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            std::strncpy(error_info->message, "Unknown exception", sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
        }
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
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call([&]() {
        m->setup_accelerated_heap(arena_base, arena_size);
    });
}

LibLoongError libloong_machine_simulate(
    LibLoongMachine* machine,
    uint64_t max_instructions,
    uint64_t counter,
    LibLoongErrorInfo* error_info)
{
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call_ex([&]() {
        m->simulate(max_instructions, counter);
    }, error_info);
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
            reinterpret_cast<Machine::syscall_t*>(*(void(**)(Machine&))&handler));
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
    uint64_t* return_value,
    LibLoongErrorInfo* error_info)
{
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call_ex([&]() {
        const address_t exit_addr = m->memory.exit_address();

        auto& cpu = m->cpu;
        cpu.reg(REG_RA) = exit_addr;
        for (size_t i = 0; i < arg_count && i < 8; i++) {
            cpu.reg(REG_A0 + i) = args[i];
        }
        cpu.registers().pc = func_addr;

        m->simulate(max_instructions, 0);
        if (m->instruction_limit_reached()) {
            throw MachineTimeoutException();
        }

        if (return_value)
            *return_value = cpu.reg(REG_A0);
    }, error_info);
}

LibLoongError libloong_machine_vmcall_by_name(
    LibLoongMachine* machine,
    const char* func_name,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    uint64_t* return_value,
    LibLoongErrorInfo* error_info)
{
    Machine* m = reinterpret_cast<Machine*>(machine);

    uint64_t addr = m->address_of(func_name);
    if (addr == 0) {
        if (error_info) {
            error_info->error_code = LIBLOONG_ERROR_SYMBOL_NOT_FOUND;
            error_info->exception_type = LIBLOONG_EXCEPTION_NONE;
            error_info->data = 0;
            std::snprintf(error_info->message, sizeof(error_info->message), "Symbol not found: %s", func_name);
        }
        return LIBLOONG_ERROR_SYMBOL_NOT_FOUND;
    }

    return libloong_machine_vmcall(machine, addr, max_instructions, args, arg_count, return_value, error_info);
}

LibLoongError libloong_machine_vmcall_float(
    LibLoongMachine* machine,
    uint64_t func_addr,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    float* return_value,
    LibLoongErrorInfo* error_info)
{
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call_ex([&]() {
        const address_t exit_addr = m->memory.exit_address();

        auto& cpu = m->cpu;
        cpu.reg(REG_RA) = exit_addr;
        for (size_t i = 0; i < arg_count && i < 8; i++) {
            cpu.reg(REG_A0 + i) = args[i];
        }
        cpu.registers().pc = func_addr;

        m->simulate(max_instructions, 0);
        if (m->instruction_limit_reached()) {
            throw MachineTimeoutException();
        }

        if (return_value)
            *return_value = cpu.registers().getfl32(REG_FA0);
    }, error_info);
}

LibLoongError libloong_machine_vmcall_double(
    LibLoongMachine* machine,
    uint64_t func_addr,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    double* return_value,
    LibLoongErrorInfo* error_info)
{
    Machine* m = reinterpret_cast<Machine*>(machine);

    return safe_call_ex([&]() {
        const address_t exit_addr = m->memory.exit_address();

        auto& cpu = m->cpu;
        cpu.reg(REG_RA) = exit_addr;
        for (size_t i = 0; i < arg_count && i < 8; i++) {
            cpu.reg(REG_A0 + i) = args[i];
        }
        cpu.registers().pc = func_addr;

        m->simulate(max_instructions, 0);
        if (m->instruction_limit_reached()) {
            throw MachineTimeoutException();
        }

        if (return_value)
            *return_value = cpu.registers().getfl64(REG_FA0);
    }, error_info);
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

LibLoongError libloong_machine_copy_to_guest(LibLoongMachine* machine, uint64_t dest, const void* src, size_t len) {
    Machine* m = reinterpret_cast<Machine*>(machine);
    return safe_call([&]() {
        m->memory.copy_to_guest(dest, src, len);
    }, LIBLOONG_ERROR_INVALID_ADDRESS);
}

LibLoongError libloong_machine_copy_from_guest(const LibLoongMachine* machine, void* dest, uint64_t src, size_t len) {
    const Machine* m = reinterpret_cast<const Machine*>(machine);
    return safe_call([&]() {
        m->memory.copy_from_guest(dest, src, len);
    }, LIBLOONG_ERROR_INVALID_ADDRESS);
}

uint64_t libloong_machine_mmap_allocate(LibLoongMachine* machine, size_t size) {
    Machine* m = reinterpret_cast<Machine*>(machine);
    return m->memory.mmap_allocate(size);
}

uint64_t libloong_machine_arena_malloc(LibLoongMachine* machine, size_t size) {
    Machine* m = reinterpret_cast<Machine*>(machine);
    if (!m->has_arena()) {
        return 0;
    }
    return m->arena().malloc(size);
}

int libloong_machine_arena_free(LibLoongMachine* machine, uint64_t ptr) {
    Machine* m = reinterpret_cast<Machine*>(machine);
    if (!m->has_arena()) {
        return -1;
    }
    return m->arena().free(ptr);
}

int libloong_machine_has_arena(const LibLoongMachine* machine) {
    const Machine* m = reinterpret_cast<const Machine*>(machine);
    return m->has_arena() ? 1 : 0;
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
