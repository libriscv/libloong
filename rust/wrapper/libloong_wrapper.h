#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types
typedef struct LibLoongMachine LibLoongMachine;

// Machine options
typedef struct {
    size_t memory_max;
    size_t stack_size;
    size_t brk_size;
    int verbose_loader;
    int verbose_syscalls;
    int use_shared_execute_segments;
} LibLoongMachineOptions;

// Exception types
typedef enum {
    LIBLOONG_OK = 0,
    LIBLOONG_ERROR_INVALID_ELF = 1,
    LIBLOONG_ERROR_EXECUTION = 2,
    LIBLOONG_ERROR_TIMEOUT = 3,
    LIBLOONG_ERROR_INVALID_ADDRESS = 4,
    LIBLOONG_ERROR_SYMBOL_NOT_FOUND = 5,
    LIBLOONG_ERROR_OUT_OF_MEMORY = 6,
    LIBLOONG_ERROR_UNKNOWN = 99,
} LibLoongError;

// Detailed exception information
typedef enum {
    LIBLOONG_EXCEPTION_NONE = 0,
    LIBLOONG_EXCEPTION_ILLEGAL_OPCODE,
    LIBLOONG_EXCEPTION_ILLEGAL_OPERATION,
    LIBLOONG_EXCEPTION_PROTECTION_FAULT,
    LIBLOONG_EXCEPTION_EXECUTION_SPACE_PROTECTION_FAULT,
    LIBLOONG_EXCEPTION_MISALIGNED_INSTRUCTION,
    LIBLOONG_EXCEPTION_UNIMPLEMENTED_INSTRUCTION,
    LIBLOONG_EXCEPTION_MACHINE_TIMEOUT,
    LIBLOONG_EXCEPTION_OUT_OF_MEMORY,
    LIBLOONG_EXCEPTION_INVALID_PROGRAM,
    LIBLOONG_EXCEPTION_FEATURE_DISABLED,
    LIBLOONG_EXCEPTION_UNIMPLEMENTED_SYSCALL,
    LIBLOONG_EXCEPTION_GUEST_ABORT,
} LibLoongExceptionType;

// Extended error information
typedef struct {
    LibLoongError error_code;
    LibLoongExceptionType exception_type;
    uint64_t data;  // Context-dependent: failing address, PC, or other relevant data
    char message[256];
} LibLoongErrorInfo;

// Syscall callback type
typedef void (*LibLoongSyscallHandler)(LibLoongMachine* machine);

// stdout callback type
typedef void (*LibLoongStdoutCallback)(const char* data, size_t len);

// Machine creation and destruction
LibLoongMachine* libloong_machine_create(
    const uint8_t* binary_data,
    size_t binary_size,
    const LibLoongMachineOptions* options,
    LibLoongErrorInfo* error_info
);

void libloong_machine_destroy(LibLoongMachine* machine);

// Setup functions
LibLoongError libloong_machine_setup_linux(
    LibLoongMachine* machine,
    const char** args,
    size_t argc,
    const char** env,
    size_t envc
);

void libloong_machine_setup_minimal_syscalls(void);
void libloong_machine_setup_linux_syscalls(void);

LibLoongError libloong_machine_setup_accelerated_syscalls(LibLoongMachine* machine);
LibLoongError libloong_machine_setup_accelerated_heap(
    LibLoongMachine* machine,
    uint64_t arena_base,
    size_t arena_size
);

// Execution
LibLoongError libloong_machine_simulate(
    LibLoongMachine* machine,
    uint64_t max_instructions,
    uint64_t counter,
    LibLoongErrorInfo* error_info
);

void libloong_machine_stop(LibLoongMachine* machine);
int libloong_machine_stopped(const LibLoongMachine* machine);
int libloong_machine_instruction_limit_reached(const LibLoongMachine* machine);

// Instruction counter
uint64_t libloong_machine_instruction_counter(const LibLoongMachine* machine);
void libloong_machine_set_instruction_counter(LibLoongMachine* machine, uint64_t val);
void libloong_machine_increment_counter(LibLoongMachine* machine, uint64_t val);
uint64_t libloong_machine_max_instructions(const LibLoongMachine* machine);
void libloong_machine_set_max_instructions(LibLoongMachine* machine, uint64_t val);

// System call interface
void libloong_install_syscall_handler(unsigned sysnum, LibLoongSyscallHandler handler);
void libloong_machine_system_call(LibLoongMachine* machine, unsigned sysnum);

// System call arguments
uint64_t libloong_machine_sysarg(const LibLoongMachine* machine, int idx);
void libloong_machine_set_result(LibLoongMachine* machine, uint64_t value);
uint64_t libloong_machine_return_value(const LibLoongMachine* machine);

// Function calls (vmcall)
LibLoongError libloong_machine_vmcall(
    LibLoongMachine* machine,
    uint64_t func_addr,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    uint64_t* return_value,
    LibLoongErrorInfo* error_info
);

LibLoongError libloong_machine_vmcall_by_name(
    LibLoongMachine* machine,
    const char* func_name,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    uint64_t* return_value,
    LibLoongErrorInfo* error_info
);

// vmcall with float return support
LibLoongError libloong_machine_vmcall_float(
    LibLoongMachine* machine,
    uint64_t func_addr,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    float* return_value,
    LibLoongErrorInfo* error_info
);

LibLoongError libloong_machine_vmcall_double(
    LibLoongMachine* machine,
    uint64_t func_addr,
    uint64_t max_instructions,
    const uint64_t* args,
    size_t arg_count,
    double* return_value,
    LibLoongErrorInfo* error_info
);

// Symbol lookup
uint64_t libloong_machine_address_of(const LibLoongMachine* machine, const char* name);
int libloong_machine_has_symbol(const LibLoongMachine* machine, const char* name);

// Memory access
LibLoongError libloong_machine_read_memory(
    const LibLoongMachine* machine,
    uint64_t addr,
    void* data,
    size_t size
);

LibLoongError libloong_machine_write_memory(
    LibLoongMachine* machine,
    uint64_t addr,
    const void* data,
    size_t size
);

LibLoongError libloong_machine_read_string(
    const LibLoongMachine* machine,
    uint64_t addr,
    char* buffer,
    size_t max_len,
    size_t* actual_len
);

// CPU register access
uint64_t libloong_machine_get_register(const LibLoongMachine* machine, unsigned reg_num);
void libloong_machine_set_register(LibLoongMachine* machine, unsigned reg_num, uint64_t value);
uint64_t libloong_machine_get_pc(const LibLoongMachine* machine);
void libloong_machine_set_pc(LibLoongMachine* machine, uint64_t pc);

// Floating-point register access
float libloong_machine_get_float_register(const LibLoongMachine* machine, unsigned reg_num);
void libloong_machine_set_float_register(LibLoongMachine* machine, unsigned reg_num, float value);
double libloong_machine_get_double_register(const LibLoongMachine* machine, unsigned reg_num);
void libloong_machine_set_double_register(LibLoongMachine* machine, unsigned reg_num, double value);

// Memory operations
LibLoongError libloong_machine_copy_to_guest(LibLoongMachine* machine, uint64_t dest, const void* src, size_t len);
LibLoongError libloong_machine_copy_from_guest(const LibLoongMachine* machine, void* dest, uint64_t src, size_t len);
uint64_t libloong_machine_mmap_allocate(LibLoongMachine* machine, size_t size);

// Guest heap allocation (requires accelerated heap to be set up)
uint64_t libloong_machine_arena_malloc(LibLoongMachine* machine, size_t size);
int libloong_machine_arena_free(LibLoongMachine* machine, uint64_t ptr);
int libloong_machine_has_arena(const LibLoongMachine* machine);

// Print callback
void libloong_machine_set_stdout_callback(LibLoongStdoutCallback callback);

// User data
void libloong_machine_set_userdata(LibLoongMachine* machine, void* userdata);
void* libloong_machine_get_userdata(const LibLoongMachine* machine);

// Error handling
const char* libloong_error_string(LibLoongError error);

// Helper: get default options
LibLoongMachineOptions libloong_default_options(void);

#ifdef __cplusplus
}
#endif
