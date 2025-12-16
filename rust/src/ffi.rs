//! Low-level FFI bindings to the libloong C wrapper

use std::fmt;

#[repr(C)]
pub struct LibLoongMachine {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct LibLoongMachineOptions {
    pub memory_max: usize,
    pub stack_size: usize,
    pub brk_size: usize,
    pub verbose_loader: i32,
    pub verbose_syscalls: i32,
    pub use_shared_execute_segments: i32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LibLoongError {
    LIBLOONG_OK = 0,
    LIBLOONG_ERROR_INVALID_ELF = 1,
    LIBLOONG_ERROR_EXECUTION = 2,
    LIBLOONG_ERROR_TIMEOUT = 3,
    LIBLOONG_ERROR_INVALID_ADDRESS = 4,
    LIBLOONG_ERROR_SYMBOL_NOT_FOUND = 5,
    LIBLOONG_ERROR_OUT_OF_MEMORY = 6,
    LIBLOONG_ERROR_UNKNOWN = 99,
}

/// Error type for libloong operations
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error {
    /// Invalid ELF binary
    InvalidElf,
    /// Execution error
    Execution,
    /// Instruction limit exceeded (timeout)
    Timeout,
    /// Invalid memory address
    InvalidAddress,
    /// Symbol not found
    SymbolNotFound,
    /// Out of memory
    OutOfMemory,
    /// Unknown error
    Unknown,
}

impl From<LibLoongError> for Error {
    fn from(err: LibLoongError) -> Self {
        match err {
            LibLoongError::LIBLOONG_OK => unreachable!("Cannot convert OK to Error"),
            LibLoongError::LIBLOONG_ERROR_INVALID_ELF => Error::InvalidElf,
            LibLoongError::LIBLOONG_ERROR_EXECUTION => Error::Execution,
            LibLoongError::LIBLOONG_ERROR_TIMEOUT => Error::Timeout,
            LibLoongError::LIBLOONG_ERROR_INVALID_ADDRESS => Error::InvalidAddress,
            LibLoongError::LIBLOONG_ERROR_SYMBOL_NOT_FOUND => Error::SymbolNotFound,
            LibLoongError::LIBLOONG_ERROR_OUT_OF_MEMORY => Error::OutOfMemory,
            LibLoongError::LIBLOONG_ERROR_UNKNOWN => Error::Unknown,
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::InvalidElf => write!(f, "Invalid ELF binary"),
            Error::Execution => write!(f, "Execution error"),
            Error::Timeout => write!(f, "Instruction limit exceeded"),
            Error::InvalidAddress => write!(f, "Invalid memory address"),
            Error::SymbolNotFound => write!(f, "Symbol not found"),
            Error::OutOfMemory => write!(f, "Out of memory"),
            Error::Unknown => write!(f, "Unknown error"),
        }
    }
}

impl std::error::Error for Error {}

pub type LibLoongSyscallHandler = extern "C" fn(*mut LibLoongMachine);
pub type LibLoongStdoutCallback = extern "C" fn(*const i8, usize);

#[link(name = "loong_wrapper", kind = "static")]
extern "C" {
    pub fn libloong_machine_create(
        binary_data: *const u8,
        binary_size: usize,
        options: *const LibLoongMachineOptions,
        error: *mut LibLoongError,
    ) -> *mut LibLoongMachine;

    pub fn libloong_machine_destroy(machine: *mut LibLoongMachine);

    pub fn libloong_machine_setup_linux(
        machine: *mut LibLoongMachine,
        args: *const *const i8,
        argc: usize,
        env: *const *const i8,
        envc: usize,
    ) -> LibLoongError;

    pub fn libloong_machine_setup_minimal_syscalls();
    pub fn libloong_machine_setup_linux_syscalls();

    pub fn libloong_machine_setup_accelerated_syscalls(
        machine: *mut LibLoongMachine,
    ) -> LibLoongError;

    pub fn libloong_machine_setup_accelerated_heap(
        machine: *mut LibLoongMachine,
        arena_base: u64,
        arena_size: usize,
    ) -> LibLoongError;

    pub fn libloong_machine_simulate(
        machine: *mut LibLoongMachine,
        max_instructions: u64,
        counter: u64,
    ) -> LibLoongError;

    pub fn libloong_machine_stop(machine: *mut LibLoongMachine);
    pub fn libloong_machine_stopped(machine: *const LibLoongMachine) -> i32;
    pub fn libloong_machine_instruction_limit_reached(machine: *const LibLoongMachine) -> i32;

    pub fn libloong_machine_instruction_counter(machine: *const LibLoongMachine) -> u64;
    pub fn libloong_machine_set_instruction_counter(machine: *mut LibLoongMachine, val: u64);
    pub fn libloong_machine_increment_counter(machine: *mut LibLoongMachine, val: u64);
    pub fn libloong_machine_max_instructions(machine: *const LibLoongMachine) -> u64;
    pub fn libloong_machine_set_max_instructions(machine: *mut LibLoongMachine, val: u64);

    pub fn libloong_install_syscall_handler(sysnum: u32, handler: LibLoongSyscallHandler);
    pub fn libloong_machine_system_call(machine: *mut LibLoongMachine, sysnum: u32);

    pub fn libloong_machine_sysarg(machine: *const LibLoongMachine, idx: i32) -> u64;
    pub fn libloong_machine_set_result(machine: *mut LibLoongMachine, value: u64);
    pub fn libloong_machine_return_value(machine: *const LibLoongMachine) -> u64;

    pub fn libloong_machine_vmcall(
        machine: *mut LibLoongMachine,
        func_addr: u64,
        max_instructions: u64,
        args: *const u64,
        arg_count: usize,
        return_value: *mut u64,
    ) -> LibLoongError;

    pub fn libloong_machine_vmcall_by_name(
        machine: *mut LibLoongMachine,
        func_name: *const i8,
        max_instructions: u64,
        args: *const u64,
        arg_count: usize,
        return_value: *mut u64,
    ) -> LibLoongError;

    pub fn libloong_machine_address_of(machine: *const LibLoongMachine, name: *const i8) -> u64;
    pub fn libloong_machine_has_symbol(machine: *const LibLoongMachine, name: *const i8) -> i32;

    pub fn libloong_machine_read_memory(
        machine: *const LibLoongMachine,
        addr: u64,
        data: *mut u8,
        size: usize,
    ) -> LibLoongError;

    pub fn libloong_machine_write_memory(
        machine: *mut LibLoongMachine,
        addr: u64,
        data: *const u8,
        size: usize,
    ) -> LibLoongError;

    pub fn libloong_machine_read_string(
        machine: *const LibLoongMachine,
        addr: u64,
        buffer: *mut i8,
        max_len: usize,
        actual_len: *mut usize,
    ) -> LibLoongError;

    pub fn libloong_machine_get_register(machine: *const LibLoongMachine, reg_num: u32) -> u64;
    pub fn libloong_machine_set_register(machine: *mut LibLoongMachine, reg_num: u32, value: u64);
    pub fn libloong_machine_get_pc(machine: *const LibLoongMachine) -> u64;
    pub fn libloong_machine_set_pc(machine: *mut LibLoongMachine, pc: u64);

    pub fn libloong_machine_set_stdout_callback(callback: Option<LibLoongStdoutCallback>);

    pub fn libloong_machine_set_userdata(machine: *mut LibLoongMachine, userdata: *mut ());
    pub fn libloong_machine_get_userdata(machine: *const LibLoongMachine) -> *mut ();

    pub fn libloong_error_string(error: LibLoongError) -> *const i8;
    pub fn libloong_default_options() -> LibLoongMachineOptions;
}
