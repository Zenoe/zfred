#ifndef LOG_H_
#define LOG_H_
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h> // For Visual Studio Output window

#include "stringutil.h"
void setup_logging()
{
    // Log to file
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("zfred_spd.log", true);
    // Log to Visual Studio Output window
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{file_sink, msvc_sink};
    auto logger = std::make_shared<spdlog::logger>("clipboard", sinks.begin(), sinks.end());
    spdlog::set_default_logger(logger);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S][%l][thread %t] %v");
    spdlog::set_level(spdlog::level::info); // change to debug/trace for more details
}

template<typename... Args>
void log_info_w(const std::wstring& fmt, Args&&... args) {
    spdlog::info(string_util::wstring_to_utf8(fmt).c_str(), args...);
    //spdlog::flush();
}

// Usage
// Call setup_logging() once at startup, before logging anything.

/* void log_example(int item_id) */
/* { */
/*     spdlog::info("Added clipboard item: {}", item_id); */
/*     spdlog::error("DB error: something went wrong"); */
/* } */


#endif // LOG_H_
