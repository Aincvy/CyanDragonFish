/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include "config.h"
#include <string_view>

#include <atomic>

#include "network.h"
#include "player.h"

#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>

#define SERVER_VERSION 1
#define SERVER_VERSION_STR "0.1"


namespace cdf {

    struct Server {
        LaunchConfig launchConfig;
        PlayerManager playerManager;
        NetworkManager networkManager;

        void shutdownServer();
        bool isShutdown() const;
        bool isRunning() const;

        mongocxx::pool::entry acquireDbClient();

        void initDatabase();

    private:
        std::atomic_bool running = true;

        mongocxx::pool* dbPool = nullptr;
    };


    int initServer(std::string_view configPath);

    Server* currentServer();

}