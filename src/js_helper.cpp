#include "log.h"
#include "js_helper.h"

#include <spdlog/spdlog.h>
#include <v8.h>
#include <v8-platform.h>
#include <libplatform/libplatform.h>


namespace cdf {

    void initJsEngine(std::string_view execPath) {
        SPDLOG_INFO("Init Js Engine, execPath: {}", execPath);
        auto path = execPath.data();
        // Initialize V8.
        v8::V8::InitializeICUDefaultLocation(path);
        v8::V8::InitializeExternalStartupData(path);
        std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(platform.get());
        v8::V8::Initialize();

        SPDLOG_INFO("Init Js Engine Over");
    }

    void destroyJsEngine() {
        SPDLOG_INFO("Destroy Js Engine");
        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();
        SPDLOG_INFO("Destroy Js Engine Over");
    }

    
}