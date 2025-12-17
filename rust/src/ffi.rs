//! Low-level FFI bindings to the libloong C wrapper
#![allow(dead_code)]
#![allow(non_camel_case_types)]

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

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LibLoongExceptionType {
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
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct LibLoongErrorInfo {
    pub error_code: LibLoongError,
    pub exception_type: LibLoongExceptionType,
    pub data: u64,
    pub message: [i8; 256],
}

/// Exception details from the guest
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ExceptionType {
    IllegalOpcode,
    IllegalOperation,
    ProtectionFault,
    ExecutionSpaceProtectionFault,
    MisalignedInstruction,
    UnimplementedInstruction,
    MachineTimeout,
    OutOfMemory,
    InvalidProgram,
    FeatureDisabled,
    UnimplementedSyscall,
    GuestAbort,
}

impl From<LibLoongExceptionType> for ExceptionType {
    fn from(ex: LibLoongExceptionType) -> Self {
        match ex {
            LibLoongExceptionType::LIBLOONG_EXCEPTION_ILLEGAL_OPCODE => {
                ExceptionType::IllegalOpcode
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_ILLEGAL_OPERATION => {
                ExceptionType::IllegalOperation
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_PROTECTION_FAULT => {
                ExceptionType::ProtectionFault
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_EXECUTION_SPACE_PROTECTION_FAULT => {
                ExceptionType::ExecutionSpaceProtectionFault
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_MISALIGNED_INSTRUCTION => {
                ExceptionType::MisalignedInstruction
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_UNIMPLEMENTED_INSTRUCTION => {
                ExceptionType::UnimplementedInstruction
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_MACHINE_TIMEOUT => {
                ExceptionType::MachineTimeout
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_OUT_OF_MEMORY => ExceptionType::OutOfMemory,
            LibLoongExceptionType::LIBLOONG_EXCEPTION_INVALID_PROGRAM => {
                ExceptionType::InvalidProgram
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_FEATURE_DISABLED => {
                ExceptionType::FeatureDisabled
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_UNIMPLEMENTED_SYSCALL => {
                ExceptionType::UnimplementedSyscall
            }
            LibLoongExceptionType::LIBLOONG_EXCEPTION_GUEST_ABORT => ExceptionType::GuestAbort,
            _ => ExceptionType::IllegalOperation,
        }
    }
}

/// Error type for libloong operations
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Error {
    /// Invalid ELF binary
    InvalidElf(String),
    /// Execution error with exception details
    Execution {
        exception_type: Option<ExceptionType>,
        data: u64,
        message: String,
    },
    /// Instruction limit exceeded (timeout)
    Timeout { data: u64, message: String },
    /// Invalid memory address
    InvalidAddress { address: u64, message: String },
    /// Symbol not found
    SymbolNotFound(String),
    /// Out of memory
    OutOfMemory(String),
    /// Unknown error
    Unknown(String),
}

impl From<LibLoongError> for Error {
    fn from(err: LibLoongError) -> Self {
        match err {
            LibLoongError::LIBLOONG_OK => unreachable!("Cannot convert OK to Error"),
            LibLoongError::LIBLOONG_ERROR_INVALID_ELF => {
                Error::InvalidElf("Invalid ELF binary".to_string())
            }
            LibLoongError::LIBLOONG_ERROR_EXECUTION => Error::Execution {
                exception_type: None,
                data: 0,
                message: "Execution error".to_string(),
            },
            LibLoongError::LIBLOONG_ERROR_TIMEOUT => Error::Timeout {
                data: 0,
                message: "Instruction limit exceeded".to_string(),
            },
            LibLoongError::LIBLOONG_ERROR_INVALID_ADDRESS => Error::InvalidAddress {
                address: 0,
                message: "Invalid memory address".to_string(),
            },
            LibLoongError::LIBLOONG_ERROR_SYMBOL_NOT_FOUND => {
                Error::SymbolNotFound("Symbol not found".to_string())
            }
            LibLoongError::LIBLOONG_ERROR_OUT_OF_MEMORY => {
                Error::OutOfMemory("Out of memory".to_string())
            }
            LibLoongError::LIBLOONG_ERROR_UNKNOWN => Error::Unknown("Unknown error".to_string()),
        }
    }
}

impl From<LibLoongErrorInfo> for Error {
    fn from(info: LibLoongErrorInfo) -> Self {
        // Convert C string to Rust String
        let message = unsafe {
            let bytes = std::slice::from_raw_parts(info.message.as_ptr() as *const u8, 256);
            let len = bytes.iter().position(|&b| b == 0).unwrap_or(256);
            String::from_utf8_lossy(&bytes[..len]).to_string()
        };

        match info.error_code {
            LibLoongError::LIBLOONG_OK => unreachable!("Cannot convert OK to Error"),
            LibLoongError::LIBLOONG_ERROR_INVALID_ELF => Error::InvalidElf(message),
            LibLoongError::LIBLOONG_ERROR_EXECUTION => Error::Execution {
                exception_type: if info.exception_type
                    != LibLoongExceptionType::LIBLOONG_EXCEPTION_NONE
                {
                    Some(info.exception_type.into())
                } else {
                    None
                },
                data: info.data,
                message,
            },
            LibLoongError::LIBLOONG_ERROR_TIMEOUT => Error::Timeout {
                data: info.data,
                message,
            },
            LibLoongError::LIBLOONG_ERROR_INVALID_ADDRESS => Error::InvalidAddress {
                address: info.data,
                message,
            },
            LibLoongError::LIBLOONG_ERROR_SYMBOL_NOT_FOUND => Error::SymbolNotFound(message),
            LibLoongError::LIBLOONG_ERROR_OUT_OF_MEMORY => Error::OutOfMemory(message),
            LibLoongError::LIBLOONG_ERROR_UNKNOWN => Error::Unknown(message),
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::InvalidElf(msg) => write!(f, "Invalid ELF binary: {}", msg),
            Error::Execution {
                exception_type,
                data,
                message,
            } => {
                write!(f, "Execution error")?;
                if let Some(ex) = exception_type {
                    write!(f, " ({:?})", ex)?;
                }
                write!(f, ": {} (data: 0x{:x})", message, data)
            }
            Error::Timeout { data, message } => write!(
                f,
                "Instruction limit exceeded: {} (at PC: 0x{:x})",
                message, data
            ),
            Error::InvalidAddress { address, message } => {
                write!(f, "Invalid memory address 0x{:x}: {}", address, message)
            }
            Error::SymbolNotFound(msg) => write!(f, "Symbol not found: {}", msg),
            Error::OutOfMemory(msg) => write!(f, "Out of memory: {}", msg),
            Error::Unknown(msg) => write!(f, "Unknown error: {}", msg),
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
        error_info: *mut LibLoongErrorInfo,
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
        error_info: *mut LibLoongErrorInfo,
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
        error_info: *mut LibLoongErrorInfo,
    ) -> LibLoongError;

    pub fn libloong_machine_vmcall_by_name(
        machine: *mut LibLoongMachine,
        func_name: *const i8,
        max_instructions: u64,
        args: *const u64,
        arg_count: usize,
        return_value: *mut u64,
        error_info: *mut LibLoongErrorInfo,
    ) -> LibLoongError;

    pub fn libloong_machine_vmcall_float(
        machine: *mut LibLoongMachine,
        func_addr: u64,
        max_instructions: u64,
        args: *const u64,
        arg_count: usize,
        return_value: *mut f32,
        error_info: *mut LibLoongErrorInfo,
    ) -> LibLoongError;

    pub fn libloong_machine_vmcall_double(
        machine: *mut LibLoongMachine,
        func_addr: u64,
        max_instructions: u64,
        args: *const u64,
        arg_count: usize,
        return_value: *mut f64,
        error_info: *mut LibLoongErrorInfo,
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

    pub fn libloong_machine_get_float_register(
        machine: *const LibLoongMachine,
        reg_num: u32,
    ) -> f32;
    pub fn libloong_machine_set_float_register(
        machine: *mut LibLoongMachine,
        reg_num: u32,
        value: f32,
    );
    pub fn libloong_machine_get_double_register(
        machine: *const LibLoongMachine,
        reg_num: u32,
    ) -> f64;
    pub fn libloong_machine_set_double_register(
        machine: *mut LibLoongMachine,
        reg_num: u32,
        value: f64,
    );

    pub fn libloong_machine_copy_to_guest(
        machine: *mut LibLoongMachine,
        dest: u64,
        src: *const u8,
        len: usize,
    ) -> LibLoongError;
    pub fn libloong_machine_copy_from_guest(
        machine: *const LibLoongMachine,
        dest: *mut u8,
        src: u64,
        len: usize,
    ) -> LibLoongError;
    pub fn libloong_machine_mmap_allocate(machine: *mut LibLoongMachine, size: usize) -> u64;

    pub fn libloong_machine_arena_malloc(machine: *mut LibLoongMachine, size: usize) -> u64;
    pub fn libloong_machine_arena_free(machine: *mut LibLoongMachine, ptr: u64) -> i32;
    pub fn libloong_machine_has_arena(machine: *const LibLoongMachine) -> i32;

    pub fn libloong_machine_set_stdout_callback(callback: Option<LibLoongStdoutCallback>);

    pub fn libloong_machine_set_userdata(machine: *mut LibLoongMachine, userdata: *mut ());
    pub fn libloong_machine_get_userdata(machine: *const LibLoongMachine) -> *mut ();

    pub fn libloong_error_string(error: LibLoongError) -> *const i8;
    pub fn libloong_default_options() -> LibLoongMachineOptions;
}
