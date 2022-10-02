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
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/console.log", 1048576 * 5, 3);

        logConsole = new spdlog::logger("console", {console_sink, fileSink});
        logConsole->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#(%!)] [%t] [%^%l%$] %v");

#ifdef CyanDragonFishDebug
        console_sink->set_level(spdlog::level::debug);
        fileSink->set_level(spdlog::level::debug);
        logConsole->set_level(spdlog::level::debug);
#else
        console_sink->set_level(spdlog::level::info);
        fileSink->set_level(spdlog::level::info);
#endif // CyanDragonFishDebug

        spdlog::set_default_logger(std::shared_ptr<spdlog::logger>(logConsole));
    }

    void initErrorLog() {
        auto console_err_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console_err_sink->set_level(spdlog::level::err);
        
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/error.log", 1048576 * 5, 3);
        fileSink->set_level(spdlog::level::err);

        logError = new spdlog::logger("error", {console_err_sink, fileSink});
        logError->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#(%!)] [%t] [%^%l%$] %v");
    }

    void initMessageLog() {
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/message.log", 1048576 * 5, 3);
        fileSink->set_level(spdlog::level::debug);

        logMessage = new spdlog::logger("message", fileSink);
        logMessage->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%s:%#(%!)] [%t] [%^%l%$] %v");

#ifdef CyanDragonFishDebug
        logMessage->set_level(spdlog::level::debug);
        logMessage->flush_on(spdlog::level::debug);
#else

#endif // CyanDragonFishDebug

    }

    void initLog(){
        // spdlog::cfg::load_env_levels();

        initConsoleLog();
        SPDLOG_LOGGER_INFO(logConsole, "console log is in position.");
        logConsole->flush();

        initErrorLog();
        initMessageLog();
    }

    void flushLog() {
        if(logConsole){
            logConsole->flush();
        }
        if (logMessage) {
            logMessage->flush();
        }
        if (logError) {
            logError->flush();
        }
    }

}