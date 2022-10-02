#include "log.h"
#include "server.h"
#include "stdio.h"
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "network.h"
#include <string_view>


namespace cdf {

    int launch(std::string_view configPath) {
        initLog();
        logConsole->info("program start!");
        initServer(configPath);

        currentServer()->shutdownServer();
        return 0;
    }

}

int main(int argc, const char** argv) {
    return cdf::launch(argc > 1 ? argv[1] : "./config.yaml");
}