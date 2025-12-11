#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace loongarch::script {

struct ScriptDepthMeter {
	ScriptDepthMeter(uint8_t& val) : m_val(++val) {}
	~ScriptDepthMeter() { m_val --; }

	uint8_t get() const noexcept { return m_val; }
	bool is_one() const noexcept { return m_val == 1; }

private:
	uint8_t& m_val;
};

// Forward declarations
class HostCallbackManager;

class ScriptException : public std::runtime_error {
public:
    explicit ScriptException(const std::string& msg) : std::runtime_error(msg) {}
};

class CompilationException : public ScriptException {
public:
    explicit CompilationException(const std::string& msg) : ScriptException(msg) {}
};

class ExecutionException : public ScriptException {
public:
    explicit ExecutionException(const std::string& msg) : ScriptException(msg) {}
};



} // loongarch::script
