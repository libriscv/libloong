#include "shared_exec_segment.hpp"
#include "util/crc32.hpp"
#include <mutex>

namespace loongarch
{
	// Create a SegmentKey from a DecodedExecuteSegment
	SegmentKey SegmentKey::from(address_t begin, uint32_t crc32c,  uint64_t arena_size)
	{
		SegmentKey key;
		key.pc = begin;
		key.arena_size = arena_size;
		key.crc = crc32c;

		return key;
	}

	SegmentKey SegmentKey::from(const DecodedExecuteSegment& segment, uint64_t arena_size)
	{
		return from(segment.exec_begin(), segment.crc32c_hash(), arena_size);
	}

	// SharedExecuteSegments implementation

	void SharedExecuteSegments::remove_if_unique(key_t key)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// We are not able to remove the Segment itself, as the mutex
		// may be locked by another thread. We can, however, lock the
		// Segment's mutex and set the segment to nullptr if it's the last reference.
		auto it = m_segments.find(key);
		if (it != m_segments.end()) {
			std::lock_guard<std::mutex> seg_lock(it->second.mutex);
			if (it->second.segment && it->second.segment.use_count() == 1) {
				it->second.segment = nullptr;
			}
		}
	}

	SharedExecuteSegments::Segment& SharedExecuteSegments::get_segment(key_t key)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto& entry = m_segments[key];
		return entry;
	}

	// Global singleton
	SharedExecuteSegments& get_shared_execute_segments()
	{
		static SharedExecuteSegments instance;
		return instance;
	}

} // namespace loongarch
