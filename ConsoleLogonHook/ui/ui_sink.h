#pragma once
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/fwd.h"

struct LogMessage
{
	std::string msg;
	spdlog::level::level_enum level;
};

namespace log_global
{
	inline bool g_shouldConsoleScrollDown = false;
	inline std::vector<LogMessage> g_logs;
}

template<typename Mutex>
class CUiSink : public spdlog::sinks::base_sink<Mutex>
{
public:
	explicit CUiSink() {}
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		log_global::g_shouldConsoleScrollDown = true;

		spdlog::memory_buf_t formatted;
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

		log_global::g_logs.push_back({
			fmt::to_string(formatted),
			msg.level
			});
	}

	void flush_() override
	{
	}
};

#include "spdlog/details/null_mutex.h"
#include <mutex>
using ui_sink_mt = CUiSink<std::mutex>;
using ui_sink_st = CUiSink<spdlog::details::null_mutex>;