/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include "config.h"
#include <string_view>

#include "network.h"
#include "player.h"

namespace cdf {
    struct Server {
        LaunchConfig launchConfig;
        PlayerManager playerManager;
        NetworkManager networkManager;
    };


    int initServer(std::string_view configPath);

    Server* currentServer();

}