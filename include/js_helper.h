/**
 * created by aincvy(aincvy@gmail.com) on 2022/10/3
 */

#pragma once

#include <string_view>
#include <v8.h>

#include "player.h"

namespace cdf {

    void initJsEngine(std::string_view execPath);

    void destroyJsEngine();

    /**
     * Domain classes.
     */
    void registerDomain(v8::Isolate* isolate);

    /**
     * Database operations.
     */
    void registerDatabaseOpr(v8::Isolate* isolate, PlayerThreadLocal* threadLocal);

    void registerUtilFunctions(v8::Isolate* isolate);

    /**
     * load and execute all js files in `javascripts` folder.
     */
    void loadJsFiles(v8::Isolate* isolate);
}