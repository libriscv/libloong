#include "decoded_exec_segment.hpp"
#include "machine.hpp"

namespace loongarch
{

#ifdef LA_BINARY_TRANSLATION
	// Forward declaration from tr_compiler.cpp
	void dylib_close(void* dylib, bool is_libtcc);

	bool DecodedExecuteSegment::is_background_compiling() const noexcept
	{
		std::lock_guard<std::mutex> lock(m_background_compilation_mutex);
		return m_is_background_compiling;
	}

	void DecodedExecuteSegment::set_background_compiling(bool is_bg)
	{
		std::lock_guard<std::mutex> lock(m_background_compilation_mutex);
		if (m_is_background_compiling && !is_bg) {
			// Notify any waiting threads that compilation is complete
			m_background_compilation_cv.notify_all();
		}
		m_is_background_compiling = is_bg;
	}

	void DecodedExecuteSegment::wait_for_compilation_complete()
	{
		std::unique_lock<std::mutex> lock(m_background_compilation_mutex);
		m_background_compilation_cv.wait(lock, [this] {
			return !m_is_background_compiling;
		});
	}
#endif

	DecodedExecuteSegment::~DecodedExecuteSegment()
	{
#ifdef LA_BINARY_TRANSLATION
		// Close the dynamic library if we have one
		if (m_bintr_dl) {
			dylib_close(m_bintr_dl, m_is_libtcc);
			m_bintr_dl = nullptr;
		}

		// Wait for any background compilation to complete
		wait_for_compilation_complete();

		// Clean up patched decoder cache
		if (m_patched_decoder_cache.cache) {
			delete[] m_patched_decoder_cache.cache;
			m_patched_decoder_cache.cache = nullptr;
		}
#endif

		// Clean up main decoder cache
		if (m_decoder_cache.cache) {
			delete[] m_decoder_cache.cache;
			m_decoder_cache.cache = nullptr;
		}
	}

} // namespace loongarch
