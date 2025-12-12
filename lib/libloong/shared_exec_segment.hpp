#pragma once
#include "common.hpp"
#include "decoded_exec_segment.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace loongarch
{
	// Key for identifying unique execute segments
	// Two segments are considered identical if they have the same:
	// - Base address (pc)
	// - Content hash (crc32c)
	// - Arena size (for binary translation compatibility)
	struct SegmentKey {
		address_t pc;
		uint32_t crc;
		uint64_t arena_size;

		static SegmentKey from(address_t begin, uint32_t crc32c, uint64_t arena_size);
		static SegmentKey from(const DecodedExecuteSegment& segment, uint64_t arena_size);

		bool operator==(const SegmentKey& other) const {
			return pc == other.pc && crc == other.crc;
		}

		bool operator<(const SegmentKey& other) const {
			return pc < other.pc || (pc == other.pc && crc < other.crc);
		}
	};

} // namespace loongarch

// Hash function for SegmentKey
namespace std {
	template <>
	struct hash<loongarch::SegmentKey> {
		size_t operator()(const loongarch::SegmentKey& key) const {
			return key.pc ^ key.crc ^ key.arena_size;
		}
	};
}

namespace loongarch
{
	// Thread-safe shared execute segment storage
	// Manages a global cache of decoded execute segments that can be shared
	// across multiple Machine instances running the same program.
	struct SharedExecuteSegments {
		SharedExecuteSegments() = default;
		SharedExecuteSegments(const SharedExecuteSegments&) = delete;
		SharedExecuteSegments& operator=(const SharedExecuteSegments&) = delete;

		using key_t = SegmentKey;

		// Wrapper around a shared execute segment with its own mutex
		// This allows fine-grained locking: the global map is locked only
		// during lookup/insertion, while individual segments can be accessed
		// concurrently by different threads.
		struct Segment {
			std::shared_ptr<DecodedExecuteSegment> segment;
			std::mutex mutex;

			// Thread-safe getter
			std::shared_ptr<DecodedExecuteSegment> get() {
				std::lock_guard<std::mutex> lock(mutex);
				return segment;
			}

			// Internal setter (caller must hold mutex)
			void unlocked_set(std::shared_ptr<DecodedExecuteSegment> seg) {
				this->segment = std::move(seg);
			}
		};

		// Remove a segment if it is the last reference
		// This is called when a Machine is being destroyed and wants to
		// clean up its execute segments. If the segment is still referenced
		// by other machines, it will be kept alive.
		void remove_if_unique(key_t key);

		// Get or create a segment entry
		// Returns a reference to the Segment wrapper in the map.
		// The caller should lock the Segment's mutex before accessing it.
		Segment& get_segment(key_t key);

		// Statistics
		size_t size() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_segments.size();
		}

		void clear() {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_segments.clear();
		}

	private:
		std::unordered_map<key_t, Segment> m_segments;
		mutable std::mutex m_mutex;
	};

	// Global shared segment cache
	// This is a singleton that manages all shared execute segments
	SharedExecuteSegments& get_shared_execute_segments();

} // namespace loongarch
