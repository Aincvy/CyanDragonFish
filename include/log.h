/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#ifdef CyanDragonFishDebug
#define SPDLOG_ACTIVE_LEVEL 0
#endif // CyanDragonFishDebug

#include "spdlog/spdlog.h"
#include <spdlog/logger.h>

namespace cdf {

    extern spdlog::logger* logConsole;
    extern spdlog::logger* logError;
    extern spdlog::logger* logMessage;
    extern spdlog::logger* logService;
    extern spdlog::logger* logScript;

    void initLog();
    
    /**
     * sync log to file.
     */
    void flushLog();

}