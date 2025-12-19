// Package libloong provides Go bindings for the libloong LoongArch emulator.
//
// This package allows you to load and execute 64-bit LoongArch ELF binaries,
// call guest functions (vmcall), and interact with guest memory from Go.
package libloong

// #cgo CFLAGS: -I${SRCDIR}/../rust/wrapper -I${SRCDIR}/../lib -I${SRCDIR}/../build/lib
// #cgo CXXFLAGS: -std=c++20 -I${SRCDIR}/../rust/wrapper -I${SRCDIR}/../lib -I${SRCDIR}/../build/lib
// #cgo LDFLAGS: -L${SRCDIR}/../build/lib -L${SRCDIR} -lloong_wrapper -lloong -lstdc++ -lm
// #include <stdlib.h>
// #include <string.h>
// #include "libloong_wrapper.h"
import "C"
import (
	"fmt"
	"runtime"
	"unsafe"
)

// MachineOptions configures a new Machine instance
type MachineOptions struct {
	MemoryMax                uint64 // Maximum memory size in bytes
	StackSize                uint64 // Stack size in bytes
	BrkSize                  uint64 // Heap (brk) size in bytes
	VerboseLoader            bool   // Print loader debug messages
	VerboseSyscalls          bool   // Print syscall debug messages
	UseSharedExecuteSegments bool   // Share execute segments across instances
}

// DefaultOptions returns sensible default options for machine creation
func DefaultOptions() MachineOptions {
	opts := C.libloong_default_options()
	return MachineOptions{
		MemoryMax:                uint64(opts.memory_max),
		StackSize:                uint64(opts.stack_size),
		BrkSize:                  uint64(opts.brk_size),
		VerboseLoader:            opts.verbose_loader != 0,
		VerboseSyscalls:          opts.verbose_syscalls != 0,
		UseSharedExecuteSegments: opts.use_shared_execute_segments != 0,
	}
}

// Error types
type ErrorCode int

const (
	ErrorOK ErrorCode = iota
	ErrorInvalidELF
	ErrorExecution
	ErrorTimeout
	ErrorInvalidAddress
	ErrorSymbolNotFound
	ErrorOutOfMemory
	ErrorUnknown ErrorCode = 99
)

func (e ErrorCode) String() string {
	switch e {
	case ErrorOK:
		return "Success"
	case ErrorInvalidELF:
		return "Invalid ELF binary"
	case ErrorExecution:
		return "Execution error"
	case ErrorTimeout:
		return "Instruction limit exceeded"
	case ErrorInvalidAddress:
		return "Invalid memory address"
	case ErrorSymbolNotFound:
		return "Symbol not found"
	case ErrorOutOfMemory:
		return "Out of memory"
	default:
		return "Unknown error"
	}
}

// ExceptionType represents detailed exception information
type ExceptionType int

const (
	ExceptionNone ExceptionType = iota
	ExceptionIllegalOpcode
	ExceptionIllegalOperation
	ExceptionProtectionFault
	ExceptionExecutionSpaceProtectionFault
	ExceptionMisalignedInstruction
	ExceptionUnimplementedInstruction
	ExceptionMachineTimeout
	ExceptionOutOfMemory
	ExceptionInvalidProgram
	ExceptionFeatureDisabled
	ExceptionUnimplementedSyscall
	ExceptionGuestAbort
)

// Error represents a libloong error with detailed context
type Error struct {
	Code      ErrorCode
	Exception ExceptionType
	Data      uint64
	Message   string
}

func (e *Error) Error() string {
	if e.Message != "" {
		return fmt.Sprintf("%s: %s (data: 0x%x)", e.Code, e.Message, e.Data)
	}
	return e.Code.String()
}

// Machine represents a LoongArch emulator instance
type Machine struct {
	handle *C.LibLoongMachine
}

// NewMachine creates a new Machine from an ELF binary
func NewMachine(binary []byte, options *MachineOptions) (*Machine, error) {
	if len(binary) == 0 {
		return nil, &Error{
			Code:    ErrorInvalidELF,
			Message: "Empty binary data",
		}
	}

	var opts C.LibLoongMachineOptions
	if options != nil {
		opts.memory_max = C.size_t(options.MemoryMax)
		opts.stack_size = C.size_t(options.StackSize)
		opts.brk_size = C.size_t(options.BrkSize)
		opts.verbose_loader = boolToInt(options.VerboseLoader)
		opts.verbose_syscalls = boolToInt(options.VerboseSyscalls)
		opts.use_shared_execute_segments = boolToInt(options.UseSharedExecuteSegments)
	} else {
		opts = C.libloong_default_options()
	}

	var errorInfo C.LibLoongErrorInfo
	handle := C.libloong_machine_create(
		(*C.uint8_t)(unsafe.Pointer(&binary[0])),
		C.size_t(len(binary)),
		&opts,
		&errorInfo,
	)

	if handle == nil {
		return nil, convertError(&errorInfo)
	}

	m := &Machine{handle: handle}
	runtime.SetFinalizer(m, (*Machine).destroy)
	return m, nil
}

// destroy frees the machine resources
func (m *Machine) destroy() {
	if m.handle != nil {
		C.libloong_machine_destroy(m.handle)
		m.handle = nil
	}
}

// Close explicitly frees machine resources
func (m *Machine) Close() {
	m.destroy()
	runtime.SetFinalizer(m, nil)
}

// SetupLinux initializes the Linux environment with arguments and environment variables
func (m *Machine) SetupLinux(args []string, env []string) error {
	cArgs := make([]*C.char, len(args))
	for i, arg := range args {
		cArgs[i] = C.CString(arg)
		defer C.free(unsafe.Pointer(cArgs[i]))
	}

	cEnv := make([]*C.char, len(env))
	for i, e := range env {
		cEnv[i] = C.CString(e)
		defer C.free(unsafe.Pointer(cEnv[i]))
	}

	var cArgsPtr **C.char
	if len(cArgs) > 0 {
		cArgsPtr = &cArgs[0]
	}

	var cEnvPtr **C.char
	if len(cEnv) > 0 {
		cEnvPtr = &cEnv[0]
	}

	result := C.libloong_machine_setup_linux(
		m.handle,
		cArgsPtr,
		C.size_t(len(cArgs)),
		cEnvPtr,
		C.size_t(len(cEnv)),
	)

	if result != C.LIBLOONG_OK {
		return &Error{Code: ErrorCode(result)}
	}
	return nil
}

// SetupLinuxSyscalls installs Linux syscall handlers (global)
func SetupLinuxSyscalls() {
	C.libloong_machine_setup_linux_syscalls()
}

// SetupMinimalSyscalls installs minimal syscall handlers (global)
func SetupMinimalSyscalls() {
	C.libloong_machine_setup_minimal_syscalls()
}

// SetupAcceleratedSyscalls sets up accelerated syscalls for this machine
func (m *Machine) SetupAcceleratedSyscalls() error {
	result := C.libloong_machine_setup_accelerated_syscalls(m.handle)
	if result != C.LIBLOONG_OK {
		return &Error{Code: ErrorCode(result)}
	}
	return nil
}

// SetupAcceleratedHeap sets up an accelerated heap arena
func (m *Machine) SetupAcceleratedHeap(base uint64, size uint64) error {
	result := C.libloong_machine_setup_accelerated_heap(
		m.handle,
		C.uint64_t(base),
		C.size_t(size),
	)
	if result != C.LIBLOONG_OK {
		return &Error{Code: ErrorCode(result)}
	}
	return nil
}

// Simulate executes the guest program for up to maxInstructions
func (m *Machine) Simulate(maxInstructions uint64) error {
	var errorInfo C.LibLoongErrorInfo
	result := C.libloong_machine_simulate(
		m.handle,
		C.uint64_t(maxInstructions),
		0,
		&errorInfo,
	)

	if result != C.LIBLOONG_OK {
		return convertError(&errorInfo)
	}
	return nil
}

// Stop stops the machine execution
func (m *Machine) Stop() {
	C.libloong_machine_stop(m.handle)
}

// Stopped returns true if the machine has stopped
func (m *Machine) Stopped() bool {
	return C.libloong_machine_stopped(m.handle) != 0
}

// InstructionLimitReached returns true if the instruction limit was reached
func (m *Machine) InstructionLimitReached() bool {
	return C.libloong_machine_instruction_limit_reached(m.handle) != 0
}

// InstructionCounter returns the current instruction counter
func (m *Machine) InstructionCounter() uint64 {
	return uint64(C.libloong_machine_instruction_counter(m.handle))
}

// SetInstructionCounter sets the instruction counter
func (m *Machine) SetInstructionCounter(val uint64) {
	C.libloong_machine_set_instruction_counter(m.handle, C.uint64_t(val))
}

// IncrementCounter increments the instruction counter
func (m *Machine) IncrementCounter(val uint64) {
	C.libloong_machine_increment_counter(m.handle, C.uint64_t(val))
}

// MaxInstructions returns the maximum instruction limit
func (m *Machine) MaxInstructions() uint64 {
	return uint64(C.libloong_machine_max_instructions(m.handle))
}

// SetMaxInstructions sets the maximum instruction limit
func (m *Machine) SetMaxInstructions(val uint64) {
	C.libloong_machine_set_max_instructions(m.handle, C.uint64_t(val))
}

// ReturnValue returns the integer return value (register $a0)
// This is the value in register $a0 (r4) after a vmcall or simulate
func (m *Machine) ReturnValue() uint64 {
	return uint64(C.libloong_machine_return_value(m.handle))
}

// ReturnValueFloat32 returns the 32-bit float return value
// This is the value in register $fa0 after a vmcall
func (m *Machine) ReturnValueFloat32() float32 {
	return float32(C.libloong_machine_get_float_register(m.handle, 0)) // FA0 = FPR 0
}

// ReturnValueFloat64 returns the 64-bit float return value
// This is the value in register $fa0 after a vmcall
func (m *Machine) ReturnValueFloat64() float64 {
	return float64(C.libloong_machine_get_double_register(m.handle, 0)) // FA0 = FPR 0
}

// VMCall calls a guest function by address
//
// Use ReturnValue(), ReturnValueFloat32(), or ReturnValueFloat64() to retrieve
// the function's return value after calling.
//
// Examples:
//
//	// Call by address
//	err := machine.VMCall(0x12000, math.MaxUint64, 42, 13)
//	result := machine.ReturnValue()
//
//	// Get float return value
//	err := machine.VMCall(addr, math.MaxUint64)
//	result := machine.ReturnValueFloat32()
func (m *Machine) VMCall(addr uint64, maxInstructions uint64, args ...uint64) error {
	var cArgs *C.uint64_t
	if len(args) > 0 {
		cArgs = (*C.uint64_t)(unsafe.Pointer(&args[0]))
	}

	var returnValue C.uint64_t
	var errorInfo C.LibLoongErrorInfo

	result := C.libloong_machine_vmcall(
		m.handle,
		C.uint64_t(addr),
		C.uint64_t(maxInstructions),
		cArgs,
		C.size_t(len(args)),
		&returnValue,
		&errorInfo,
	)

	if result != C.LIBLOONG_OK {
		return convertError(&errorInfo)
	}
	return nil
}

// VMCallByName calls a guest function by symbol name
//
// Use ReturnValue(), ReturnValueFloat32(), or ReturnValueFloat64() to retrieve
// the function's return value after calling.
//
// Examples:
//
//	// Call by name
//	err := machine.VMCallByName("factorial", math.MaxUint64, 5)
//	result := machine.ReturnValue()
func (m *Machine) VMCallByName(name string, maxInstructions uint64, args ...uint64) error {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var cArgs *C.uint64_t
	if len(args) > 0 {
		cArgs = (*C.uint64_t)(unsafe.Pointer(&args[0]))
	}

	var returnValue C.uint64_t
	var errorInfo C.LibLoongErrorInfo

	result := C.libloong_machine_vmcall_by_name(
		m.handle,
		cName,
		C.uint64_t(maxInstructions),
		cArgs,
		C.size_t(len(args)),
		&returnValue,
		&errorInfo,
	)

	if result != C.LIBLOONG_OK {
		return convertError(&errorInfo)
	}
	return nil
}

// AddressOf looks up a symbol and returns its address
func (m *Machine) AddressOf(name string) uint64 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	return uint64(C.libloong_machine_address_of(m.handle, cName))
}

// HasSymbol returns true if the symbol exists
func (m *Machine) HasSymbol(name string) bool {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	return C.libloong_machine_has_symbol(m.handle, cName) != 0
}

// ReadMemory reads data from guest memory
func (m *Machine) ReadMemory(addr uint64, size uint64) ([]byte, error) {
	data := make([]byte, size)
	result := C.libloong_machine_read_memory(
		m.handle,
		C.uint64_t(addr),
		unsafe.Pointer(&data[0]),
		C.size_t(size),
	)

	if result != C.LIBLOONG_OK {
		return nil, &Error{Code: ErrorCode(result)}
	}
	return data, nil
}

// WriteMemory writes data to guest memory
func (m *Machine) WriteMemory(addr uint64, data []byte) error {
	if len(data) == 0 {
		return nil
	}

	result := C.libloong_machine_write_memory(
		m.handle,
		C.uint64_t(addr),
		unsafe.Pointer(&data[0]),
		C.size_t(len(data)),
	)

	if result != C.LIBLOONG_OK {
		return &Error{Code: ErrorCode(result)}
	}
	return nil
}

// ReadString reads a null-terminated string from guest memory
func (m *Machine) ReadString(addr uint64, maxLen uint64) (string, error) {
	buffer := make([]byte, maxLen)
	var actualLen C.size_t

	result := C.libloong_machine_read_string(
		m.handle,
		C.uint64_t(addr),
		(*C.char)(unsafe.Pointer(&buffer[0])),
		C.size_t(maxLen),
		&actualLen,
	)

	if result != C.LIBLOONG_OK {
		return "", &Error{Code: ErrorCode(result)}
	}
	return string(buffer[:actualLen]), nil
}

// GetRegister reads a general-purpose register (0-31)
func (m *Machine) GetRegister(regNum uint) uint64 {
	return uint64(C.libloong_machine_get_register(m.handle, C.uint(regNum)))
}

// SetRegister writes a general-purpose register (0-31)
func (m *Machine) SetRegister(regNum uint, value uint64) {
	C.libloong_machine_set_register(m.handle, C.uint(regNum), C.uint64_t(value))
}

// GetPC returns the program counter
func (m *Machine) GetPC() uint64 {
	return uint64(C.libloong_machine_get_pc(m.handle))
}

// SetPC sets the program counter
func (m *Machine) SetPC(pc uint64) {
	C.libloong_machine_set_pc(m.handle, C.uint64_t(pc))
}

// GetFloatRegister reads a 32-bit floating-point register
func (m *Machine) GetFloatRegister(regNum uint) float32 {
	return float32(C.libloong_machine_get_float_register(m.handle, C.uint(regNum)))
}

// SetFloatRegister writes a 32-bit floating-point register
func (m *Machine) SetFloatRegister(regNum uint, value float32) {
	C.libloong_machine_set_float_register(m.handle, C.uint(regNum), C.float(value))
}

// GetDoubleRegister reads a 64-bit floating-point register
func (m *Machine) GetDoubleRegister(regNum uint) float64 {
	return float64(C.libloong_machine_get_double_register(m.handle, C.uint(regNum)))
}

// SetDoubleRegister writes a 64-bit floating-point register
func (m *Machine) SetDoubleRegister(regNum uint, value float64) {
	C.libloong_machine_set_double_register(m.handle, C.uint(regNum), C.double(value))
}

// CopyToGuest copies data from host to guest memory
func (m *Machine) CopyToGuest(dest uint64, src []byte) error {
	if len(src) == 0 {
		return nil
	}

	result := C.libloong_machine_copy_to_guest(
		m.handle,
		C.uint64_t(dest),
		unsafe.Pointer(&src[0]),
		C.size_t(len(src)),
	)

	if result != C.LIBLOONG_OK {
		return &Error{Code: ErrorCode(result)}
	}
	return nil
}

// CopyFromGuest copies data from guest to host memory
func (m *Machine) CopyFromGuest(src uint64, size uint64) ([]byte, error) {
	dest := make([]byte, size)
	result := C.libloong_machine_copy_from_guest(
		m.handle,
		unsafe.Pointer(&dest[0]),
		C.uint64_t(src),
		C.size_t(size),
	)

	if result != C.LIBLOONG_OK {
		return nil, &Error{Code: ErrorCode(result)}
	}
	return dest, nil
}

// MmapAllocate allocates writable guest memory via mmap
func (m *Machine) MmapAllocate(size uint64) uint64 {
	return uint64(C.libloong_machine_mmap_allocate(m.handle, C.size_t(size)))
}

// ArenaMalloc allocates memory from the accelerated heap
func (m *Machine) ArenaMalloc(size uint64) uint64 {
	return uint64(C.libloong_machine_arena_malloc(m.handle, C.size_t(size)))
}

// ArenaFree frees memory allocated from the accelerated heap
func (m *Machine) ArenaFree(ptr uint64) bool {
	return C.libloong_machine_arena_free(m.handle, C.uint64_t(ptr)) == 0
}

// HasArena returns true if the accelerated heap is set up
func (m *Machine) HasArena() bool {
	return C.libloong_machine_has_arena(m.handle) != 0
}

// SetUserdata stores arbitrary user data with the machine
func (m *Machine) SetUserdata(data unsafe.Pointer) {
	C.libloong_machine_set_userdata(m.handle, data)
}

// GetUserdata retrieves user data stored with the machine
func (m *Machine) GetUserdata() unsafe.Pointer {
	return C.libloong_machine_get_userdata(m.handle)
}

// Helper functions

func boolToInt(b bool) C.int {
	if b {
		return 1
	}
	return 0
}

func convertError(errorInfo *C.LibLoongErrorInfo) error {
	return &Error{
		Code:      ErrorCode(errorInfo.error_code),
		Exception: ExceptionType(errorInfo.exception_type),
		Data:      uint64(errorInfo.data),
		Message:   C.GoString(&errorInfo.message[0]),
	}
}
