#include "threads.hpp"
#include "../machine.hpp"
#include <algorithm>

namespace loongarch
{

MultiThreading::MultiThreading(Machine& mach)
	: machine(mach)
{
	// Best guess for default stack boundaries
	const address_t base = 0x1000;
	const address_t size = mach.cpu.reg(REG_SP) - base;
	// Create the main thread
	auto it = m_threads.try_emplace(0, *this, 0, 0x0, mach.cpu.reg(REG_SP), base, size);
	m_current = &it.first->second;
}

Thread* MultiThreading::get_thread()
{
	return this->m_current;
}

Thread* MultiThreading::get_thread(int tid)
{
	auto it = m_threads.find(tid);
	if (it == m_threads.end()) return nullptr;
	return &it->second;
}

void MultiThreading::wakeup_next()
{
	// resume a waiting thread
	if (!m_suspended.empty()) {
		auto* next = m_suspended.front();
		m_suspended.erase(m_suspended.begin());
		// resume next thread
		next->resume();
	} else {
		auto* next = get_thread(0);
		next->resume();
	}
}

Thread::Thread(
	MultiThreading& mt, int ttid, address_t tls,
	address_t stack, address_t stkbase, address_t stksize)
	 : threading(mt), tid(ttid), stack_base(stkbase), stack_size(stksize)
{
	this->stored_regs.get(REG_TP) = tls;
	this->stored_regs.get(REG_SP) = stack;
}

Thread::Thread(
	MultiThreading& mt, const Thread& other)
	: threading(mt), tid(other.tid),
	  stack_base(other.stack_base), stack_size(other.stack_size),
	  clear_tid(other.clear_tid), block_word(other.block_word), block_extra(other.block_extra)
{
	stored_regs = other.stored_regs;
}

void Thread::activate()
{
	threading.m_current = this;
	auto& cpu = threading.machine.cpu;
	cpu.reg(REG_TP) = this->stored_regs.get(REG_TP);
	cpu.reg(REG_SP) = this->stored_regs.get(REG_SP);
}

bool Thread::exit()
{
	const bool exiting_myself = (threading.get_thread() == this);
	// Copy of reference to thread manager and thread ID
	auto& thr = this->threading;
	const int tid = this->tid;
	// CLONE_CHILD_CLEARTID: set userspace TID value to zero
	if (this->clear_tid) {
		threading.machine.memory.write<address_t>(this->clear_tid, 0);
	}
	// Delete this thread (except main thread)
	if (tid != 0) {
		threading.erase_thread(tid);

		// Resume next thread in suspended list
		// Exiting main thread is a "process exit", so we don't wakeup_next
		if (exiting_myself) {
			thr.wakeup_next();
		}
	}

	// tid == 0: Main thread exited
	return (tid == 0);
}

void Thread::suspend()
{
	// copy all regs
	this->stored_regs = threading.machine.cpu.registers();
	// add to suspended (NB: can throw)
	threading.m_suspended.push_back(this);
}

void Thread::suspend(address_t return_value)
{
	this->suspend();
	// set the *future* return value for this thread
	this->stored_regs.get(REG_A0) = return_value;
}

void Thread::block(uint32_t reason, uint32_t extra)
{
	// copy all regs
	this->stored_regs = threading.machine.cpu.registers();
	this->block_word = reason;
	this->block_extra = extra;
	// add to blocked (NB: can throw)
	threading.m_blocked.push_back(this);
}

void Thread::block_return(address_t return_value, uint32_t reason, uint32_t extra)
{
	this->block(reason, extra);
	// set the block reason as the next return value
	this->stored_regs.get(REG_A0) = return_value;
}

void Thread::resume()
{
	threading.m_current = this;
	auto& m = threading.machine;
	// restore registers
	m.cpu.registers() = this->stored_regs;
	// this will ensure PC is executable in all cases
	m.cpu.aligned_jump(m.cpu.pc());
}

Thread* MultiThreading::create(
		int flags, address_t ctid, address_t ptid,
		address_t stack, address_t tls, address_t stkbase, address_t stksize)
{
	if (this->m_threads.size() >= this->m_max_threads)
		throw MachineException(INVALID_PROGRAM, "Too many threads", this->m_max_threads);

	const int tid = ++this->m_thread_counter;
	auto it = m_threads.try_emplace(tid, *this, tid, tls, stack, stkbase, stksize);
	auto* thread = &it.first->second;

	// flag for write child TID
	if (flags & CHILD_SETTID) {
		machine.memory.write<uint32_t>(ctid, thread->tid);
	}
	if (flags & PARENT_SETTID) {
		machine.memory.write<uint32_t>(ptid, thread->tid);
	}
	if (flags & CHILD_CLEARTID) {
		thread->clear_tid = ctid;
	}

	return thread;
}

void MultiThreading::erase_thread(int tid)
{
	m_threads.erase(tid);
}

bool MultiThreading::suspend_and_yield(long result)
{
	// Suspend current thread and yield to next
	if (!m_suspended.empty()) {
		auto* current = get_thread();
		current->suspend(result);
		wakeup_next();
		return true;
	}
	return false;
}

bool MultiThreading::block(address_t retval, uint32_t reason, uint32_t extra)
{
	// Block current thread and yield to next
	if (!m_suspended.empty() || m_threads.size() > 1) {
		auto* current = get_thread();
		current->block_return(retval, reason, extra);
		wakeup_next();
		return true;
	}
	return false;
}

void MultiThreading::unblock(int tid)
{
	auto it = std::find_if(m_blocked.begin(), m_blocked.end(),
		[tid](const Thread* t) { return t->tid == tid; });
	if (it != m_blocked.end()) {
		auto* thread = *it;
		m_blocked.erase(it);
		m_suspended.push_back(thread);
	}
}

size_t MultiThreading::wakeup_blocked(size_t max, uint32_t reason, uint32_t mask)
{
	size_t awakened = 0;
	for (auto it = m_blocked.begin(); it != m_blocked.end() && awakened < max; ) {
		auto* thread = *it;
		// Check if the thread matches the reason and mask
		if (thread->block_word == reason && (thread->block_extra & mask) != 0) {
			// Move from blocked to suspended
			it = m_blocked.erase(it);
			m_suspended.push_back(thread);
			awakened++;
		} else {
			++it;
		}
	}
	return awakened;
}

bool MultiThreading::preempt()
{
	// Not implemented yet
	return false;
}

bool MultiThreading::yield_to(int tid, bool store_retval)
{
	auto* thread = get_thread(tid);
	if (thread == nullptr) return false;

	auto* current = get_thread();
	if (store_retval)
		current->suspend();

	thread->resume();
	return true;
}

int Machine::gettid()
{
	if (this->has_threads()) {
		return this->threads().get_tid();
	} else {
		return 1; // Single-threaded mode: always return 1
	}
}

} // namespace loongarch
