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
    spdlog::logger* logScript;

    static std::shared_ptr<spdlog::sinks::stderr_color_sink_mt> console_err_sink = nullptr;
    static std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> error_file_sink = nullptr;
    static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink = nullptr;


    void initConsoleLog() {
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/console.log", 1048576 * 5, 3);

        logConsole = new spdlog::logger("console", {console_sink, fileSink});
        logConsole->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] [%s:%#(%!)] %v");

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
        console_err_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        console_err_sink->set_level(spdlog::level::err);
        
        error_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/error.log", 1048576 * 5, 3);
        error_file_sink->set_level(spdlog::level::err);

        logError = new spdlog::logger("error", {console_err_sink, error_file_sink});
        logError->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] [%s:%#(%!)] %v");
    }

    void initMessageLog() {
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/message.log", 1048576 * 5, 3);
        fileSink->set_level(spdlog::level::debug);

        logMessage = new spdlog::logger("message", {fileSink,  console_err_sink, error_file_sink});
        logMessage->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] [%s:%#(%!)] %v");

#ifdef CyanDragonFishDebug
        logMessage->set_level(spdlog::level::debug);
        logMessage->flush_on(spdlog::level::debug);
#else

#endif // CyanDragonFishDebug

    }

    void initScriptLog() {
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/script.log", 1048576 * 5, 3);
        fileSink->set_level(spdlog::level::debug);

        logScript = new spdlog::logger("script", {console_sink, fileSink });
        logScript->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] %v");

#ifdef CyanDragonFishDebug
        logScript->set_level(spdlog::level::debug);
        logScript->flush_on(spdlog::level::debug);
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
        initScriptLog();
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