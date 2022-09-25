#include "log.h"

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <type_traits>

namespace cdf {

    spdlog::logger* logConsole;
    spdlog::logger* logError;
    spdlog::logger* logMessage;
    spdlog::logger* logService;

    void initConsoleLog() {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/console.log", 1048576 * 5, 3);
        fileSink->set_level(spdlog::level::debug);

        logConsole = new spdlog::logger("console", {console_sink, fileSink});
    }

    void initErrorLog() {
        auto console_err_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console_err_sink->set_level(spdlog::level::err);
        
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/error.log", 1048576 * 5, 3);
        fileSink->set_level(spdlog::level::err);

        logError = new spdlog::logger("error", {console_err_sink, fileSink});
    }

    void initLog(){
        // spdlog::cfg::load_env_levels();
        initConsoleLog();
        logConsole->info("console log is in position.");
        
        initErrorLog();
    }

}