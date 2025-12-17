//! Rust bindings for the libloong LoongArch emulator
//!
//! This crate provides safe Rust bindings to the libloong C++ library,
//! a high-performance LoongArch 64-bit emulator.
//!
//! # Example
//!
//! ```no_run
//! use libloong::{Machine, MachineOptions};
//! use std::fs;
//!
//! // Load a LoongArch ELF binary
//! let binary = fs::read("program.elf").expect("Failed to read ELF");
//!
//! // Create machine with default options
//! let mut machine = Machine::new(&binary, MachineOptions::default())
//!     .expect("Failed to create machine");
//!
//! // Setup Linux environment
//! machine.setup_linux(&["program"], &[])
//!     .expect("Failed to setup Linux");
//!
//! // Execute the program
//! machine.simulate(u64::MAX)
//!     .expect("Failed to simulate");
//!
//! println!("Instructions executed: {}", machine.instruction_counter());
//! ```

mod ffi;

pub use ffi::{Error, ExceptionType};

use std::ffi::CString;
use std::marker::PhantomData;

/// Represents either a function address or function name for vmcall operations
pub enum FunctionRef<'a> {
    /// Call function by address
    Address(u64),
    /// Call function by name
    Name(&'a str),
}

impl<'a> From<u64> for FunctionRef<'a> {
    fn from(addr: u64) -> Self {
        FunctionRef::Address(addr)
    }
}

impl<'a> From<&'a str> for FunctionRef<'a> {
    fn from(name: &'a str) -> Self {
        FunctionRef::Name(name)
    }
}

/// Machine options for configuring the emulator
#[derive(Debug, Clone)]
pub struct MachineOptions {
    /// Maximum memory size (default: 256 MB)
    pub memory_max: usize,
    /// Stack size (default: 2 MB)
    pub stack_size: usize,
    /// BRK area size (default: 1 MB)
    pub brk_size: usize,
    /// Enable verbose ELF loader output
    pub verbose_loader: bool,
    /// Enable verbose syscall logging
    pub verbose_syscalls: bool,
    /// Enable shared execute segments (reduces memory usage)
    pub use_shared_execute_segments: bool,
}

impl Default for MachineOptions {
    fn default() -> Self {
        Self {
            memory_max: 256 * 1024 * 1024,
            stack_size: 2 * 1024 * 1024,
            brk_size: 1024 * 1024,
            verbose_loader: false,
            verbose_syscalls: false,
            use_shared_execute_segments: true,
        }
    }
}

impl From<MachineOptions> for ffi::LibLoongMachineOptions {
    fn from(opts: MachineOptions) -> Self {
        Self {
            memory_max: opts.memory_max,
            stack_size: opts.stack_size,
            brk_size: opts.brk_size,
            verbose_loader: if opts.verbose_loader { 1 } else { 0 },
            verbose_syscalls: if opts.verbose_syscalls { 1 } else { 0 },
            use_shared_execute_segments: if opts.use_shared_execute_segments {
                1
            } else {
                0
            },
        }
    }
}

/// The main LoongArch emulator machine
///
/// This struct represents a complete LoongArch machine instance with CPU, memory,
/// and all necessary state for executing LoongArch 64-bit programs.
pub struct Machine {
    handle: *mut ffi::LibLoongMachine,
    _marker: PhantomData<ffi::LibLoongMachine>,
}

// Safety: The underlying C++ Machine is thread-safe for single-threaded access
unsafe impl Send for Machine {}

impl Machine {
    /// Helper to resolve a FunctionRef to an address
    fn resolve_function(&self, func: FunctionRef) -> Result<u64, Error> {
        match func {
            FunctionRef::Address(addr) => Ok(addr),
            FunctionRef::Name(name) => {
                let addr = self.address_of(name);
                if addr == 0 {
                    Err(Error::SymbolNotFound(format!(
                        "Function '{}' not found",
                        name
                    )))
                } else {
                    Ok(addr)
                }
            }
        }
    }

    /// Create a new machine from a LoongArch ELF binary
    ///
    /// # Arguments
    ///
    /// * `binary` - The ELF binary data
    /// * `options` - Machine configuration options
    ///
    /// # Errors
    ///
    /// Returns an error if the binary is invalid or machine creation fails
    pub fn new(binary: &[u8], options: MachineOptions) -> Result<Self, Error> {
        let opts = ffi::LibLoongMachineOptions::from(options);
        let mut error_info = std::mem::MaybeUninit::<ffi::LibLoongErrorInfo>::uninit();

        let handle = unsafe {
            ffi::libloong_machine_create(
                binary.as_ptr(),
                binary.len(),
                &opts,
                error_info.as_mut_ptr(),
            )
        };

        if handle.is_null() {
            return Err(unsafe { error_info.assume_init() }.into());
        }

        Ok(Self {
            handle,
            _marker: PhantomData,
        })
    }

    /// Setup Linux environment with arguments and environment variables
    ///
    /// # Arguments
    ///
    /// * `args` - Command-line arguments (argv)
    /// * `env` - Environment variables
    pub fn setup_linux(&mut self, args: &[&str], env: &[&str]) -> Result<(), Error> {
        let args_cstr: Vec<CString> = args
            .iter()
            .map(|&s| CString::new(s).expect("Invalid argument string"))
            .collect();
        let args_ptrs: Vec<*const i8> = args_cstr.iter().map(|cs| cs.as_ptr()).collect();

        let env_cstr: Vec<CString> = env
            .iter()
            .map(|&s| CString::new(s).expect("Invalid environment string"))
            .collect();
        let env_ptrs: Vec<*const i8> = env_cstr.iter().map(|cs| cs.as_ptr()).collect();

        let error = unsafe {
            ffi::libloong_machine_setup_linux(
                self.handle,
                args_ptrs.as_ptr(),
                args_ptrs.len(),
                env_ptrs.as_ptr(),
                env_ptrs.len(),
            )
        };

        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }

        Ok(())
    }

    /// Setup minimal syscalls (basic I/O only)
    pub fn setup_minimal_syscalls() {
        unsafe {
            ffi::libloong_machine_setup_minimal_syscalls();
        }
    }

    /// Setup full Linux syscalls
    pub fn setup_linux_syscalls() {
        unsafe {
            ffi::libloong_machine_setup_linux_syscalls();
        }
    }

    /// Setup accelerated syscalls (modifies decoder cache)
    pub fn setup_accelerated_syscalls(&mut self) -> Result<(), Error> {
        let error = unsafe { ffi::libloong_machine_setup_accelerated_syscalls(self.handle) };
        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }
        Ok(())
    }

    /// Setup accelerated heap with custom arena
    pub fn setup_accelerated_heap(
        &mut self,
        arena_base: u64,
        arena_size: usize,
    ) -> Result<(), Error> {
        let error = unsafe {
            ffi::libloong_machine_setup_accelerated_heap(self.handle, arena_base, arena_size)
        };
        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }
        Ok(())
    }

    /// Check if the machine has an accelerated arena/heap
    pub fn has_arena(&self) -> bool {
        unsafe { ffi::libloong_machine_has_arena(self.handle) != 0 }
    }

    /// Allocate memory on the guest heap (requires accelerated heap to be set up)
    ///
    /// # Returns
    ///
    /// Returns the guest address of the allocated memory, or 0 if allocation fails or arena is not set up.
    pub fn arena_malloc(&mut self, size: usize) -> u64 {
        unsafe { ffi::libloong_machine_arena_malloc(self.handle, size) }
    }

    /// Free memory on the guest heap (requires accelerated heap to be set up)
    ///
    /// # Returns
    ///
    /// Returns 0 on success, -1 if arena is not set up.
    pub fn arena_free(&mut self, ptr: u64) -> i32 {
        unsafe { ffi::libloong_machine_arena_free(self.handle, ptr) }
    }

    /// Simulate execution for up to `max_instructions`
    ///
    /// # Arguments
    ///
    /// * `max_instructions` - Maximum number of instructions to execute (use `u64::MAX` for unlimited)
    ///
    /// # Returns
    ///
    /// Returns `Ok(())` if execution completes or reaches the instruction limit.
    pub fn simulate(&mut self, max_instructions: u64) -> Result<(), Error> {
        let mut error_info = std::mem::MaybeUninit::<ffi::LibLoongErrorInfo>::uninit();
        let error = unsafe {
            ffi::libloong_machine_simulate(
                self.handle,
                max_instructions,
                0,
                error_info.as_mut_ptr(),
            )
        };
        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(unsafe { error_info.assume_init() }.into());
        }
        Ok(())
    }

    /// Stop execution immediately
    pub fn stop(&mut self) {
        unsafe {
            ffi::libloong_machine_stop(self.handle);
        }
    }

    /// Check if the machine has stopped
    pub fn stopped(&self) -> bool {
        unsafe { ffi::libloong_machine_stopped(self.handle) != 0 }
    }

    /// Check if the instruction limit was reached
    pub fn instruction_limit_reached(&self) -> bool {
        unsafe { ffi::libloong_machine_instruction_limit_reached(self.handle) != 0 }
    }

    /// Get the current instruction counter
    pub fn instruction_counter(&self) -> u64 {
        unsafe { ffi::libloong_machine_instruction_counter(self.handle) }
    }

    /// Set the instruction counter
    pub fn set_instruction_counter(&mut self, value: u64) {
        unsafe {
            ffi::libloong_machine_set_instruction_counter(self.handle, value);
        }
    }

    /// Increment the instruction counter
    pub fn increment_counter(&mut self, value: u64) {
        unsafe {
            ffi::libloong_machine_increment_counter(self.handle, value);
        }
    }

    /// Get the maximum instruction limit
    pub fn max_instructions(&self) -> u64 {
        unsafe { ffi::libloong_machine_max_instructions(self.handle) }
    }

    /// Set the maximum instruction limit
    pub fn set_max_instructions(&mut self, value: u64) {
        unsafe {
            ffi::libloong_machine_set_max_instructions(self.handle, value);
        }
    }

    /// Call a guest function by address or name
    ///
    /// # Arguments
    ///
    /// * `func` - Function to call (address or name)
    /// * `args` - Function arguments (up to 8 supported)
    ///
    /// # Returns
    ///
    /// Returns () on success. Use `return_value()`, `return_value_f32()`, or `return_value_f64()`
    /// to retrieve the function's return value after calling.
    ///
    /// # Examples
    ///
    /// ```no_run
    /// # use libloong::{Machine, MachineOptions};
    /// # let mut machine = Machine::new(&[], MachineOptions::default()).unwrap();
    /// // Call by address
    /// machine.vmcall(0x12000, &[42, 13]).unwrap();
    /// let result = machine.return_value();
    ///
    /// // Call by name
    /// machine.vmcall("factorial", &[5]).unwrap();
    /// let result = machine.return_value();
    ///
    /// // Get float return value
    /// machine.vmcall("sin_approx", &[]).unwrap();
    /// let result = machine.return_value_f32();
    /// ```
    pub fn vmcall<'a>(
        &mut self,
        func: impl Into<FunctionRef<'a>>,
        args: &[u64],
    ) -> Result<(), Error> {
        if args.len() > 8 {
            return Err(Error::Execution {
                exception_type: None,
                data: 0,
                message: "Too many arguments (max 8)".to_string(),
            });
        }

        let func_addr = self.resolve_function(func.into())?;
        let mut return_value: u64 = 0;
        let mut error_info = std::mem::MaybeUninit::<ffi::LibLoongErrorInfo>::uninit();
        let error = unsafe {
            ffi::libloong_machine_vmcall(
                self.handle,
                func_addr,
                u64::MAX,
                args.as_ptr(),
                args.len(),
                &mut return_value,
                error_info.as_mut_ptr(),
            )
        };

        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(unsafe { error_info.assume_init() }.into());
        }

        Ok(())
    }

    /// Get the integer return value of the last vmcall or execution
    /// This is the value in register $a0 (r4)
    pub fn return_value(&self) -> u64 {
        unsafe { ffi::libloong_machine_return_value(self.handle) }
    }

    /// Get the 32-bit float return value of the last vmcall
    /// This is the value in register $fa0
    pub fn return_value_f32(&self) -> f32 {
        unsafe { ffi::libloong_machine_get_float_register(self.handle, 0) } // FA0 = FPR 0
    }

    /// Get the 64-bit float return value of the last vmcall
    /// This is the value in register $fa0
    pub fn return_value_f64(&self) -> f64 {
        unsafe { ffi::libloong_machine_get_double_register(self.handle, 0) } // FA0 = FPR 0
    }

    /// Get the address of a symbol by name
    ///
    /// Returns 0 if the symbol is not found
    pub fn address_of(&self, name: &str) -> u64 {
        let name_cstr = match CString::new(name) {
            Ok(s) => s,
            Err(_) => return 0,
        };

        unsafe { ffi::libloong_machine_address_of(self.handle, name_cstr.as_ptr()) }
    }

    /// Check if a symbol exists
    pub fn has_symbol(&self, name: &str) -> bool {
        let name_cstr = match CString::new(name) {
            Ok(s) => s,
            Err(_) => return false,
        };

        unsafe { ffi::libloong_machine_has_symbol(self.handle, name_cstr.as_ptr()) != 0 }
    }

    /// Read memory from guest address space
    pub fn read_memory(&self, addr: u64, buffer: &mut [u8]) -> Result<(), Error> {
        let error = unsafe {
            ffi::libloong_machine_read_memory(
                self.handle,
                addr,
                buffer.as_mut_ptr() as *mut _,
                buffer.len(),
            )
        };

        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }

        Ok(())
    }

    /// Write memory to guest address space
    pub fn write_memory(&mut self, addr: u64, data: &[u8]) -> Result<(), Error> {
        let error = unsafe {
            ffi::libloong_machine_write_memory(
                self.handle,
                addr,
                data.as_ptr() as *const _,
                data.len(),
            )
        };

        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }

        Ok(())
    }

    /// Copy data from host to guest memory
    pub fn copy_to_guest(&mut self, dest: u64, src: &[u8]) -> Result<(), Error> {
        let error = unsafe {
            ffi::libloong_machine_copy_to_guest(self.handle, dest, src.as_ptr(), src.len())
        };
        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }
        Ok(())
    }

    /// Copy data from guest to host memory
    pub fn copy_from_guest(&self, dest: &mut [u8], src: u64) -> Result<(), Error> {
        let error = unsafe {
            ffi::libloong_machine_copy_from_guest(self.handle, dest.as_mut_ptr(), src, dest.len())
        };
        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }
        Ok(())
    }

    /// Allocate writable guest memory using mmap
    ///
    /// # Returns
    ///
    /// Returns the guest address of the allocated memory. This memory is writable
    /// and can be used with copy_to_guest/copy_from_guest or as arena base.
    pub fn mmap_allocate(&mut self, size: usize) -> u64 {
        unsafe { ffi::libloong_machine_mmap_allocate(self.handle, size) }
    }

    /// Read a null-terminated string from guest memory
    pub fn read_string(&self, addr: u64, max_len: usize) -> Result<String, Error> {
        let mut buffer = vec![0u8; max_len];
        let mut actual_len: usize = 0;

        let error = unsafe {
            ffi::libloong_machine_read_string(
                self.handle,
                addr,
                buffer.as_mut_ptr() as *mut i8,
                max_len,
                &mut actual_len,
            )
        };

        if error != ffi::LibLoongError::LIBLOONG_OK {
            return Err(error.into());
        }

        buffer.truncate(actual_len);
        String::from_utf8(buffer).map_err(|_| Error::Execution {
            exception_type: None,
            data: 0,
            message: "Invalid UTF-8 string".to_string(),
        })
    }

    /// Get a CPU register value (0-31)
    pub fn get_register(&self, reg_num: u32) -> u64 {
        unsafe { ffi::libloong_machine_get_register(self.handle, reg_num) }
    }

    /// Set a CPU register value (0-31)
    pub fn set_register(&mut self, reg_num: u32, value: u64) {
        unsafe {
            ffi::libloong_machine_set_register(self.handle, reg_num, value);
        }
    }

    /// Get a floating-point register value as f32 (0-31)
    pub fn get_float_register(&self, reg_num: u32) -> f32 {
        unsafe { ffi::libloong_machine_get_float_register(self.handle, reg_num) }
    }

    /// Set a floating-point register value as f32 (0-31)
    pub fn set_float_register(&mut self, reg_num: u32, value: f32) {
        unsafe {
            ffi::libloong_machine_set_float_register(self.handle, reg_num, value);
        }
    }

    /// Get a floating-point register value as f64 (0-31)
    pub fn get_double_register(&self, reg_num: u32) -> f64 {
        unsafe { ffi::libloong_machine_get_double_register(self.handle, reg_num) }
    }

    /// Set a floating-point register value as f64 (0-31)
    pub fn set_double_register(&mut self, reg_num: u32, value: f64) {
        unsafe {
            ffi::libloong_machine_set_double_register(self.handle, reg_num, value);
        }
    }

    /// Get the program counter (PC)
    pub fn get_pc(&self) -> u64 {
        unsafe { ffi::libloong_machine_get_pc(self.handle) }
    }

    /// Set the program counter (PC)
    pub fn set_pc(&mut self, pc: u64) {
        unsafe {
            ffi::libloong_machine_set_pc(self.handle, pc);
        }
    }

    /// Set user data pointer
    pub fn set_userdata(&mut self, userdata: *mut ()) {
        unsafe {
            ffi::libloong_machine_set_userdata(self.handle, userdata as *mut _);
        }
    }

    /// Get user data pointer
    pub fn get_userdata(&self) -> *mut () {
        unsafe { ffi::libloong_machine_get_userdata(self.handle) }
    }

    /// Get the raw handle (for advanced use cases)
    pub fn as_raw(&self) -> *mut ffi::LibLoongMachine {
        self.handle
    }
}

impl Drop for Machine {
    fn drop(&mut self) {
        unsafe {
            ffi::libloong_machine_destroy(self.handle);
        }
    }
}

/// Set a global stdout callback for all machines
///
/// This callback will be invoked whenever guest code writes to stdout
pub fn set_stdout_callback(callback: Option<fn(&[u8])>) {
    unsafe {
        if let Some(cb) = callback {
            // Store the Rust callback in a static
            STDOUT_CALLBACK = Some(cb);
            ffi::libloong_machine_set_stdout_callback(Some(stdout_callback_wrapper));
        } else {
            STDOUT_CALLBACK = None;
            ffi::libloong_machine_set_stdout_callback(None);
        }
    }
}

static mut STDOUT_CALLBACK: Option<fn(&[u8])> = None;

extern "C" fn stdout_callback_wrapper(data: *const i8, len: usize) {
    unsafe {
        if let Some(callback) = STDOUT_CALLBACK {
            let slice = std::slice::from_raw_parts(data as *const u8, len);
            callback(slice);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_machine_options_default() {
        let opts = MachineOptions::default();
        assert_eq!(opts.memory_max, 256 * 1024 * 1024);
        assert_eq!(opts.stack_size, 2 * 1024 * 1024);
        assert_eq!(opts.brk_size, 1024 * 1024);
    }
}
