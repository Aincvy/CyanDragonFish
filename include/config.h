/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <string_view>
#include <string>
#include <sys/types.h>
#include "common.h"

namespace cdf {

    struct LaunchConfigServer {
        // listen address
        std::string address = "0.0.0.0"; 
        ushort port = 14999;
    };

    struct LaunchConfigPlayer {
        uint maxOnline = 20;
    };

    struct LaunchConfigDatabase {

    };

    struct LaunchConfig  {
        LaunchConfigServer server;
        LaunchConfigPlayer player;
        LaunchConfigDatabase database;
    };

    int loadConfig(std::string_view path);

}