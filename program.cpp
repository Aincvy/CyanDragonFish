#include "log.h"
#include "server.h"
#include "stdio.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "network.h"
#include <string_view>

#include "executor.h"

namespace cdf {

    int launch(std::string_view configPath, std::string_view execPath) {
        initLog();
        SPDLOG_INFO("program start!");
        initExecutor();
        initServer(configPath);

        currentServer()->shutdownServer();
        return 0;
    }

}

int main(int argc, const char** argv) {
    return cdf::launch(argc > 1 ? argv[1] : "./config.yaml", argv[0]);
}