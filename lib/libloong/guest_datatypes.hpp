#pragma once
#include "native/heap.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <array>

namespace loongarch {

// Forward declarations
template <typename T>
struct GuestStdVector;

// Type trait for identifying GuestStdVector
template <typename T>
struct is_guest_stdvector : std::false_type {};

template <typename T>
struct is_guest_stdvector<GuestStdVector<T>> : std::true_type {};

// Type trait for detecting types that support fix_addresses
template <typename T, typename = void>
struct has_fix_addresses : std::false_type {};

template <typename T>
struct has_fix_addresses<T, std::void_t<decltype(std::declval<T>().fix_addresses(std::declval<Machine&>(), std::declval<address_t>()))>> : std::true_type {};

// View into libstdc++'s std::string
struct GuestStdString {
	static constexpr std::size_t SSO = 15;

	address_t ptr;
	address_t size;
	union {
		char data[SSO + 1];
		address_t capacity;
	};

	constexpr GuestStdString() noexcept : ptr(0), size(0), capacity(0) {}

	GuestStdString(Machine& machine, std::string_view str = "")
		: ptr(0), size(0), capacity(0)
	{
		this->set_string(machine, 0, str);
	}

	GuestStdString(Machine& machine, address_t self, std::string_view str = "")
		: ptr(0), size(0), capacity(0)
	{
		this->set_string(machine, self, str);
	}

	bool empty() const noexcept { return size == 0; }

	std::string to_string(const Machine& machine, std::size_t max_len = 16UL << 20) const
	{
		if (this->size <= SSO)
			return std::string(data, size);
		else if (this->size > max_len)
			throw std::runtime_error("Guest std::string too large (size > 16MB)");
		// Copy the string from guest memory
		std::string result(size, '\0');
		machine.memory.copy_from_guest(result.data(), ptr, size);
		return result;
	}

	std::string_view to_view(const Machine& machine, std::size_t max_len = 16UL << 20) const
	{
		if (this->size <= SSO)
			return std::string_view(data, size);
		else if (this->size > max_len)
			throw std::runtime_error("Guest std::string too large (size > 16MB)");
		// View the string from guest memory
		return machine.memory.memview(ptr, size);
	}

	void set_string(Machine& machine, address_t self, const void* str, std::size_t len, bool use_memarray = true)
	{
		this->free(machine);

		if (len <= SSO)
		{
			this->ptr = self + offsetof(GuestStdString, data);
			this->size = len;
			std::memcpy(this->data, str, len);
			this->data[len] = '\0';
		}
		else
		{
			this->ptr = machine.arena().malloc(len+1);
			this->size = len;
			this->capacity = len;
			if (use_memarray)
			{
				char* dst = machine.memory.template writable_memarray<char>(this->ptr, len + 1);
				std::memcpy(dst, str, len);
				dst[len] = '\0';
			}
			else
			{
				machine.memory.copy_to_guest(this->ptr, str, len);
				machine.memory.template write<uint8_t>(this->ptr + len, 0);
			}
		}
	}

	void set_string(Machine& machine, address_t self, std::string_view str)
	{
		this->set_string(machine, self, str.data(), str.size());
	}

	void move(address_t self)
	{
		if (size <= SSO) {
			this->ptr = self + offsetof(GuestStdString, data);
		}
	}

	void fix_addresses(Machine& machine, address_t self)
	{
		this->move(self);
	}

	void free(Machine& machine)
	{
		if (size > SSO) {
			machine.arena().free(ptr);
		}
		this->ptr = 0;
		this->size = 0;
	}
};

// View into Rust's String (same layout as Vec<u8>: ptr, capacity, len)
struct GuestRustString {
	address_t len;
	address_t ptr;
	address_t capacity;

	constexpr GuestRustString() noexcept : len(0), ptr(0), capacity(0) {}

	GuestRustString(Machine& machine, std::string_view str = "")
		: len(0), ptr(0), capacity(0)
	{
		this->set_string(machine, str);
	}

	bool empty() const noexcept { return len == 0; }

	std::string to_string(const Machine& machine, std::size_t max_len = 16UL << 20) const
	{
		if (this->len == 0)
			return std::string();
		else if (this->len > max_len)
			throw std::runtime_error("Guest Rust String too large (len > 16MB)");
		// Copy the string from guest memory
		std::string result(len, '\0');
		machine.memory.copy_from_guest(result.data(), ptr, len);
		return result;
	}

	std::string_view to_view(const Machine& machine, std::size_t max_len = 16UL << 20) const
	{
		if (this->len == 0)
			return std::string_view();
		else if (this->len > max_len)
			throw std::runtime_error("Guest Rust String too large (len > 16MB)");
		// View the string from guest memory
		return machine.memory.memview(ptr, len);
	}

	void set_string(Machine& machine, const void* str, std::size_t length, bool use_memarray = true)
	{
		this->free(machine);

		if (length > 0)
		{
			this->ptr = machine.arena().malloc(length);
			this->capacity = length;
			this->len = length;
			if (use_memarray)
			{
				char* dst = machine.memory.template writable_memarray<char>(this->ptr, length);
				std::memcpy(dst, str, length);
			}
			else
			{
				machine.memory.copy_to_guest(this->ptr, str, length);
			}
		}
	}

	void set_string(Machine& machine, std::string_view str)
	{
		this->set_string(machine, str.data(), str.size());
	}

	void free(Machine& machine)
	{
		if (ptr != 0) {
			machine.arena().free(ptr);
		}
		this->ptr = 0;
		this->capacity = 0;
		this->len = 0;
	}
};

// View into libstdc++ and LLVM libc++ std::vector (same layout)
template <typename T>
struct GuestStdVector {
	address_t ptr_begin;
	address_t ptr_end;
	address_t ptr_capacity;

	constexpr GuestStdVector() noexcept : ptr_begin(0), ptr_end(0), ptr_capacity(0) {}

	GuestStdVector(Machine& machine, std::size_t elements)
		: ptr_begin(0), ptr_end(0), ptr_capacity(0)
	{
		auto [array, self] = this->alloc(machine, elements);
		(void)self;
		for (std::size_t i = 0; i < elements; i++) {
			new (&array[i]) T();
		}
		// Set new end only after all elements are constructed
		this->ptr_end = this->ptr_begin + elements * sizeof(T);
	}

	GuestStdVector(Machine& machine, const std::vector<std::string>& vec)
		: ptr_begin(0), ptr_end(0), ptr_capacity(0)
	{
		static_assert(std::is_same_v<T, GuestStdString>, "GuestStdVector<T> must be a vector of GuestStdString");
		if (vec.empty())
			return;

		// Specialization for std::vector<std::string>
		auto [array, self] = this->alloc(machine, vec.size());
		(void)self;
		for (std::size_t i = 0; i < vec.size(); i++) {
			T* str = new (&array[i]) T(machine, vec[i]);
			str->move(this->ptr_begin + i * sizeof(T));
		}
		// Set new end only after all elements are constructed
		this->ptr_end = this->ptr_begin + vec.size() * sizeof(T);
	}

	GuestStdVector(Machine& machine, const std::vector<T>& vec = {})
		: ptr_begin(0), ptr_end(0), ptr_capacity(0)
	{
		if (!vec.empty())
			this->assign(machine, vec);
	}

	template <typename... Args>
	GuestStdVector(Machine& machine, const std::array<T, sizeof...(Args)>& arr)
		: GuestStdVector(machine, std::vector<T> {arr.begin(), arr.end()})
	{
	}

	GuestStdVector(GuestStdVector&& other) noexcept
		: ptr_begin(other.ptr_begin), ptr_end(other.ptr_end), ptr_capacity(other.ptr_capacity)
	{
		other.ptr_begin = 0;
		other.ptr_end = 0;
		other.ptr_capacity = 0;
	}

	// Copying is intentionally shallow/fast, in order to avoid copying/duplication
	// Use std::move if you need proper semantics
	GuestStdVector(const GuestStdVector& other) = default;
	GuestStdVector& operator=(const GuestStdVector& other) = default;

	address_t data() const noexcept { return ptr_begin; }

	std::size_t size() const noexcept {
		return size_bytes() / sizeof(T);
	}

	bool empty() const noexcept {
		return size() == 0;
	}

	std::size_t capacity() const noexcept {
		return capacity_bytes() / sizeof(T);
	}

	T& at(Machine& machine, std::size_t index, std::size_t max_bytes = 16UL << 20) {
		if (index >= size())
			throw std::out_of_range("Guest std::vector index out of range");
		return as_array(machine, max_bytes)[index];
	}

	const T& at(Machine& machine, std::size_t index, std::size_t max_bytes = 16UL << 20) const {
		if (index >= size())
			throw std::out_of_range("Guest std::vector index out of range");
		return as_array(machine, max_bytes)[index];
	}

	void push_back(Machine& machine, T&& value) {
		if (size_bytes() >= capacity_bytes())
			this->increase_capacity(machine);
		T* array = machine.memory.template writable_memarray<T>(this->data(), size() + 1);
		new (&array[size()]) T(std::move(value));
		this->ptr_end += sizeof(T);
	}

	void push_back(Machine& machine, const T& value) {
		if (size_bytes() >= capacity_bytes())
			this->increase_capacity(machine);
		T* array = machine.memory.template writable_memarray<T>(this->data(), size() + 1);
		new (&array[size()]) T(value);
		this->ptr_end += sizeof(T);
	}

	// Specialization for std::string and std::string_view
	void push_back(Machine& machine, std::string_view value) {
		static_assert(std::is_same_v<T, GuestStdString>, "GuestStdVector: T must be a GuestStdString");
		if (size_bytes() >= capacity_bytes())
			this->increase_capacity(machine);
		T* array = machine.memory.template writable_memarray<T>(this->data(), size() + 1);
		const address_t address = this->ptr_begin + size() * sizeof(T);
		new (&array[size()]) T(machine, address, value);
		this->ptr_end += sizeof(T);
	}

	// Specialization for std::vector<U>
	template <typename U>
	void push_back(Machine& machine, const std::vector<U>& value) {
		static_assert(is_guest_stdvector<T>::value, "GuestStdVector: T must be a GuestStdVector itself");
		this->push_back(machine, GuestStdVector<U>(machine, value));
	}

	void pop_back(Machine& machine) {
		if (size() == 0)
			throw std::out_of_range("Guest std::vector is empty");
		this->free_element(machine, size() - 1);
		this->ptr_end -= sizeof(T);
	}

	void append(Machine& machine, const T* values, std::size_t count) {
		if (size_bytes() + count * sizeof(T) > capacity_bytes())
			this->reserve(machine, size() + count);
		T* array = machine.memory.template writable_memarray<T>(this->data(), size() + count);
		for (std::size_t i = 0; i < count; i++)
			new (&array[size() + i]) T(values[i]);
		this->ptr_end += count * sizeof(T);
	}

	void clear(Machine& machine) {
		for (std::size_t i = 0; i < size(); i++)
			this->free_element(machine, i);
		this->ptr_end = this->ptr_begin;
	}

	address_t address_at(std::size_t index) const {
		if (index >= size())
			throw std::out_of_range("Guest std::vector index out of range");
		return ptr_begin + index * sizeof(T);
	}

	T* as_array(Machine& machine, std::size_t max_bytes = 16UL << 20) {
		if (size_bytes() > max_bytes)
			throw std::runtime_error("Guest std::vector has size > max_bytes");
		return machine.memory.template writable_memarray<T>(data(), size());
	}

	const T* as_array(const Machine& machine, std::size_t max_bytes = 16UL << 20) const {
		if (size_bytes() > max_bytes)
			throw std::runtime_error("Guest std::vector has size > max_bytes");
		return machine.memory.template memarray<T>(data(), size());
	}

	// Iterators
	auto begin(Machine& machine) { return as_array(machine); }
	auto end(Machine& machine) { return as_array(machine) + size(); }

	std::vector<T> to_vector(const Machine& machine) const {
		if (size_bytes() > capacity_bytes())
			throw std::runtime_error("Guest std::vector has size > capacity");
		// Copy the vector from guest memory
		const size_t elements = size();
		const T* array = machine.memory.template memarray<T>(data(), elements);
		return std::vector<T>(&array[0], &array[elements]);
	}

	/// @brief Specialization for std::string
	/// @param machine The LoongArch machine
	/// @return A vector of strings
	std::vector<std::string> to_string_vector(const Machine& machine) const {
		if constexpr (std::is_same_v<T, GuestStdString>) {
			std::vector<std::string> vec;
			const size_t elements = size();
			const T* array = machine.memory.template memarray<T>(data(), elements);
			vec.reserve(elements);
			for (std::size_t i = 0; i < elements; i++)
				vec.push_back(array[i].to_string(machine));
			return vec;
		} else {
			throw std::runtime_error("GuestStdVector: T must be a GuestStdString");
		}
	}

	void assign(Machine& machine, const std::vector<T>& vec)
	{
		auto [array, self] = alloc(machine, vec.size());
		(void)self;
		std::copy(vec.begin(), vec.end(), array);
		this->ptr_end = this->ptr_begin + vec.size() * sizeof(T);
	}

	void assign(Machine& machine, const T* values, std::size_t count)
	{
		auto [array, self] = alloc(machine, count);
		(void)self;
		std::copy(values, values + count, array);
		this->ptr_end = this->ptr_begin + count * sizeof(T);
	}

	void resize(Machine& machine, std::size_t new_size)
	{
		if (new_size < size()) {
			for (std::size_t i = new_size; i < size(); i++)
				this->free_element(machine, i);
			this->ptr_end = this->ptr_begin + new_size * sizeof(T);
		} else if (new_size > size()) {
			if (new_size > capacity())
				this->reserve(machine, new_size);
			T* array = machine.memory.template writable_memarray<T>(this->data(), new_size);
			for (std::size_t i = size(); i < new_size; i++)
				new (&array[i]) T();
			this->ptr_end = this->ptr_begin + new_size * sizeof(T);
		}
	}

	void reserve(Machine& machine, std::size_t elements)
	{
		if (elements <= capacity())
			return;

		GuestStdVector<T> old_vec(std::move(*this));
		// Allocate new memory
		auto [array, self] = this->alloc(machine, elements);
		(void)self;
		if (!old_vec.empty()) {
			std::copy(old_vec.as_array(machine), old_vec.as_array(machine) + old_vec.size(), array);
			// Free the old vector manually (as we don't want to call the destructor(s))
			machine.arena().free(old_vec.ptr_begin);
		}
		this->ptr_end = this->ptr_begin + old_vec.size() * sizeof(T);
		// Adjust SSO if the vector contains std::string
		if constexpr (std::is_same_v<T, GuestStdString>) {
			T* array = machine.memory.template writable_memarray<T>(this->data(), size());
			for (std::size_t i = 0; i < size(); i++)
				array[i].move(this->ptr_begin + i * sizeof(T));
		}
	}

	void free(Machine& machine) {
		if (this->ptr_begin != 0) {
			for (std::size_t i = 0; i < size(); i++)
				this->free_element(machine, i);
			machine.arena().free(this->data());
			this->ptr_begin = 0;
			this->ptr_end = 0;
			this->ptr_capacity = 0;
		}
	}

	void fix_addresses(Machine& machine, address_t self) {
		(void)self; // Not used for vectors themselves
		// Fix addresses in elements if they need it
		if constexpr (std::is_same_v<T, GuestStdString>) {
			if (this->size() > 0) {
				T* array = machine.memory.template writable_memarray<T>(this->data(), this->size());
				for (std::size_t i = 0; i < this->size(); i++) {
					array[i].fix_addresses(machine, this->data() + i * sizeof(T));
				}
			}
		} else if constexpr (is_guest_stdvector<T>::value) {
			if (this->size() > 0) {
				T* array = machine.memory.template writable_memarray<T>(this->data(), this->size());
				for (std::size_t i = 0; i < this->size(); i++) {
					array[i].fix_addresses(machine, this->data() + i * sizeof(T));
				}
			}
		}
	}

	std::size_t size_bytes() const noexcept { return ptr_end - ptr_begin; }
	std::size_t capacity_bytes() const noexcept { return ptr_capacity - ptr_begin; }

private:
	void increase_capacity(Machine& machine) {
		this->reserve(machine, capacity() * 2 + 4);
	}

	std::tuple<T*, address_t> alloc(Machine& machine, std::size_t elements) {
		this->free(machine);

		this->ptr_begin = machine.arena().malloc(elements * sizeof(T));
		this->ptr_end = this->ptr_begin;
		this->ptr_capacity = this->ptr_begin + elements * sizeof(T);
		return { machine.memory.template writable_memarray<T>(this->data(), elements), this->data() };
	}

	void free_element(Machine& machine, std::size_t index) {
		if constexpr (std::is_same_v<T, GuestStdString> || is_guest_stdvector<T>::value) {
			this->at(machine, index).free(machine);
		} else {
			this->at(machine, index).~T();
		}
	}
};

// ScopedArenaObject: RAII wrapper for guest-heap allocated objects
template <typename T>
struct ScopedArenaObject {
	template <typename... Args>
	ScopedArenaObject(Machine& machine, Args&&... args)
		: m_machine(&machine)
	{
		this->m_addr = m_machine->arena().malloc(sizeof(T));
		if (this->m_addr == 0) {
			throw std::bad_alloc();
		}
		this->m_ptr = m_machine->memory.template writable_memarray<T>(this->m_addr, 1);

		// Construct the object
		if constexpr (std::is_same_v<T, GuestStdString>) {
			new (m_ptr) T(machine, std::forward<Args>(args)...);
		} else if constexpr (is_guest_stdvector<T>::value) {
			new (m_ptr) T(machine, std::forward<Args>(args)...);
		} else {
			// Construct the object in place (as if trivially constructible)
			new (m_ptr) T{std::forward<Args>(args)...};
		}

		// Fix addresses if the type supports it
		if constexpr (has_fix_addresses<T>::value) {
			this->m_ptr->fix_addresses(machine, this->m_addr);
		}
	}

	~ScopedArenaObject() {
		this->free_standard_types();
		m_machine->arena().free(this->m_addr);
	}

	T& operator*() { return *m_ptr; }
	T* operator->() { return m_ptr; }

	address_t address() const { return m_addr; }

	ScopedArenaObject& operator=(const ScopedArenaObject&) = delete;

	ScopedArenaObject& operator=(const T& other) {
		// It's not possible for m_addr to be 0 here, as it would have thrown in the constructor
		this->free_standard_types();
		this->allocate_if_null();
		this->m_ptr = m_machine->memory.template writable_memarray<T>(this->m_addr, 1);
		new (m_ptr) T(other);
		return *this;
	}

	// Special case for std::string
	ScopedArenaObject& operator=(std::string_view other) {
		static_assert(std::is_same_v<T, GuestStdString>, "ScopedArenaObject<T> must be a GuestStdString");
		this->allocate_if_null();
		this->m_ptr->set_string(*m_machine, this->m_addr, other.data(), other.size());
		return *this;
	}

	// Special case for std::vector
	template <typename U>
	ScopedArenaObject& operator=(const std::vector<U>& other) {
		static_assert(std::is_same_v<T, GuestStdVector<U>>, "ScopedArenaObject<T> must be a GuestStdVector<U>");
		this->allocate_if_null();
		this->m_ptr->assign(*m_machine, other);
		return *this;
	}

	ScopedArenaObject& operator=(ScopedArenaObject&& other) {
		this->free_standard_types();
		this->m_machine = other.m_machine;
		this->m_addr = other.m_addr;
		this->m_ptr = other.m_ptr;
		other.m_addr = 0;
		other.m_ptr = nullptr;
		return *this;
	}

private:
	void allocate_if_null() {
		if (this->m_addr == 0) {
			this->m_addr = m_machine->arena().malloc(sizeof(T));
			if (this->m_addr == 0) {
				throw std::bad_alloc();
			}
			this->m_ptr = m_machine->memory.template writable_memarray<T>(this->m_addr, 1);
		}
	}

	void free_standard_types() {
		if constexpr (is_guest_stdvector<T>::value || std::is_same_v<T, GuestStdString>) {
			if (this->m_ptr) {
				this->m_ptr->free(*this->m_machine);
			}
		}
	}

	T* m_ptr = nullptr;
	address_t m_addr = 0;
	Machine* m_machine;
};

// Type traits for scoped objects
template <typename T>
struct is_scoped_guest_object : std::false_type {};

template <typename T>
struct is_scoped_guest_object<ScopedArenaObject<T>> : std::true_type {};

// View into Rust's Vec<T> (layout: ptr, capacity, len)
template <typename T>
struct GuestRustVector {
	address_t len;
	address_t ptr;
	address_t capacity;

	constexpr GuestRustVector() noexcept : len(0), ptr(0), capacity(0) {}

	GuestRustVector(Machine& machine, std::size_t elements)
		: len(0), ptr(0), capacity(0)
	{
		if (elements > 0) {
			this->ptr = machine.arena().malloc(elements * sizeof(T));
			this->capacity = elements;
			this->len = elements;
			T* array = machine.memory.template writable_memarray<T>(this->ptr, elements);
			for (std::size_t i = 0; i < elements; i++) {
				new (&array[i]) T();
			}
		}
	}

	GuestRustVector(Machine& machine, const std::vector<T>& vec)
		: ptr(0), capacity(0), len(0)
	{
		if (!vec.empty())
			this->assign(machine, vec);
	}

	address_t data() const noexcept { return ptr; }

	std::size_t size() const noexcept { return len; }

	bool empty() const noexcept { return len == 0; }

	std::size_t capacity_value() const noexcept { return capacity; }

	T& at(Machine& machine, std::size_t index, std::size_t max_bytes = 16UL << 20) {
		if (index >= len)
			throw std::out_of_range("Guest Rust Vec index out of range");
		if (len * sizeof(T) > max_bytes)
			throw std::runtime_error("Guest Rust Vec has size > max_bytes");
		return machine.memory.template writable_memarray<T>(ptr, len)[index];
	}

	const T& at(Machine& machine, std::size_t index, std::size_t max_bytes = 16UL << 20) const {
		if (index >= len)
			throw std::out_of_range("Guest Rust Vec index out of range");
		if (len * sizeof(T) > max_bytes)
			throw std::runtime_error("Guest Rust Vec has size > max_bytes");
		return machine.memory.template memarray<T>(ptr, len)[index];
	}

	T* as_array(Machine& machine, std::size_t max_bytes = 16UL << 20) {
		if (len * sizeof(T) > max_bytes)
			throw std::runtime_error("Guest Rust Vec has size > max_bytes");
		return machine.memory.template writable_memarray<T>(data(), len);
	}

	const T* as_array(const Machine& machine, std::size_t max_bytes = 16UL << 20) const {
		if (len * sizeof(T) > max_bytes)
			throw std::runtime_error("Guest Rust Vec has size > max_bytes");
		return machine.memory.template memarray<T>(data(), len);
	}

	std::vector<T> to_vector(const Machine& machine) const {
		if (len == 0)
			return std::vector<T>();
		const T* array = machine.memory.template memarray<T>(data(), len);
		return std::vector<T>(&array[0], &array[len]);
	}

	void assign(Machine& machine, const std::vector<T>& vec)
	{
		this->free(machine);
		if (vec.empty())
			return;

		this->ptr = machine.arena().malloc(vec.size() * sizeof(T));
		this->capacity = vec.size();
		this->len = vec.size();
		T* array = machine.memory.template writable_memarray<T>(this->ptr, vec.size());
		std::copy(vec.begin(), vec.end(), array);
	}

	void free(Machine& machine) {
		if (this->ptr != 0) {
			machine.arena().free(this->ptr);
			this->ptr = 0;
			this->capacity = 0;
			this->len = 0;
		}
	}
};

// Convenience aliases
using CppString = GuestStdString;
template <typename T>
using CppVector = GuestStdVector<T>;
using ScopedCppString = ScopedArenaObject<GuestStdString>;
template <typename T>
using ScopedCppVector = ScopedArenaObject<GuestStdVector<T>>;

using RustString = GuestRustString;
template <typename T>
using RustVector = GuestRustVector<T>;
using ScopedRustString = ScopedArenaObject<GuestRustString>;
template <typename T>
using ScopedRustVector = ScopedArenaObject<GuestRustVector<T>>;

} // namespace loongarch
