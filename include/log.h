/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include "spdlog/spdlog.h"
#include <spdlog/logger.h>

namespace cdf {

    extern spdlog::logger* logConsole;
    extern spdlog::logger* logError;
    extern spdlog::logger* logMessage;
    extern spdlog::logger* logService;

    void initLog();
    
}