#include "log/log.h"
#include "help.h"

#include <ffshit/log/logger.h>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fmt/format.h>

#if defined(_WIN64)
    #include <spdlog/sinks/wincolor_sink.h>
#else
    #include <spdlog/sinks/ansicolor_sink.h>
#endif

Log::Interface::Ptr log_interface_ptr = Log::Interface::build();

static constexpr char SPDLOG_LOG_PATTERN[] = "[%H:%M:%S.%e] %^[%=8l]%$ %v";

static void setup_console_log() {

#if defined(_WIN64)
    auto stdout_sink    = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    auto stdout_sink    = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif

    std::vector<spdlog::sink_ptr> sinks{stdout_sink};
    auto logger = std::make_shared<spdlog::async_logger>("ffshit_console", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    logger->set_pattern(SPDLOG_LOG_PATTERN);

    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
}

static void setup_console_and_file_log(const std::filesystem::path &log_path) {
#if defined(_WIN64)
    auto stdout_sink    = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    auto stdout_sink    = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
    auto file_sink      = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), true);

    std::vector<spdlog::sink_ptr> sinks{stdout_sink, file_sink};
    auto logger = std::make_shared<spdlog::async_logger>("ffshit_console_and_file", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    logger->set_pattern(SPDLOG_LOG_PATTERN);

    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
}

namespace Log {

void init() {
    spdlog::init_thread_pool(16384, 1);

    FULLFLASH::Log::Logger::init(log_interface_ptr);
}

void setup(std::filesystem::path dst_path) {
    if (dst_path.empty()) {
        setup_console_log();
    } else {
        dst_path.append("extracting.log");

        setup_console_and_file_log(dst_path);
    }
}

};
