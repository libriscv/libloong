#pragma once
#include "../common.hpp"
#include "../registers.hpp"
#include <unordered_map>
#include <vector>

namespace loongarch
{
	// Forward declarations
	struct Machine;
	struct MultiThreading;

	// Thread clone flags
	static constexpr uint32_t PARENT_SETTID  = 0x00100000; /* set the TID in the parent */
	static constexpr uint32_t CHILD_CLEARTID = 0x00200000; /* clear the TID in the child */
	static constexpr uint32_t CHILD_SETTID   = 0x01000000; /* set the TID in the child */

	struct Thread
	{
		MultiThreading& threading;
		const int tid;
		// For returning to this thread
		Registers stored_regs;
		// Base address of the stack
		address_t stack_base;
		// Size of the stack
		address_t stack_size;
		// Address zeroed when exiting
		address_t clear_tid = 0;
		// The current or last blocked word
		uint32_t block_word = 0;
		uint32_t block_extra = 0;

		Thread(MultiThreading&, int tid, address_t tls,
			address_t stack, address_t stkbase, address_t stksize);
		Thread(MultiThreading&, const Thread& other);
		bool exit(); // Returns false when we *cannot* continue
		void suspend();
		void suspend(address_t return_value);
		void block(uint32_t reason, uint32_t extra = 0);
		void block_return(address_t return_value, uint32_t reason, uint32_t extra);
		void activate();
		void resume();
	};

	struct MultiThreading
	{
		Thread* create(int flags, address_t ctid, address_t ptid,
			address_t stack, address_t tls, address_t stkbase, address_t stksize);
		int       get_tid() const noexcept { return m_current->tid; }
		Thread*   get_thread();
		Thread*   get_thread(int tid); /* or nullptr */
		bool      preempt();
		bool      suspend_and_yield(long result = 0);
		bool      yield_to(int tid, bool store_retval = false, address_t retval = 0);
		void      erase_thread(int tid);
		void      wakeup_next();
		bool      block(address_t retval, uint32_t reason, uint32_t extra = 0);
		void      unblock(int tid);
		size_t    wakeup_blocked(size_t max, uint32_t reason, uint32_t mask = ~0U);
		/* A suspended thread can at any time be resumed. */
		auto&     suspended_threads() { return m_suspended; }
		/* A blocked thread can only be resumed by unblocking it. */
		auto&     blocked_threads() { return m_blocked; }

		MultiThreading(Machine&);
		Machine& machine;
		std::vector<Thread*> m_blocked;
		std::vector<Thread*> m_suspended;
		std::unordered_map<int, Thread> m_threads;
		unsigned   m_thread_counter = 0;
		unsigned   m_max_threads = 50;
		Thread*    m_current = nullptr;
	};

} // namespace loongarch
