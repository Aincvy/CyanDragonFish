#include "log.h"

#include "server.h"
#include "common.h"
#include "config.h"
#include <cassert>

#include <climits>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#include <spdlog/spdlog.h>

namespace cdf {

    static Server* g_server = nullptr;

    int initServer(std::string_view configPath) {
        g_server = new Server();
        if(!configPath.empty()){
            int c = loadConfig(configPath);
            if (c != CODE_OK)
            {
                return c;
            }
        }
        
        // init 
        g_server->initDatabase();
        g_server->networkManager.init();
        g_server->playerManager.init();

        flushLog();

        g_server->networkManager.serv();
        return CODE_OK;
    }

    Server* currentServer() {
        return g_server;
    }

    void Server::shutdownServer() {
        running = false;

        networkManager.detroy();
        playerManager.destroy();

        if (dbPool) {
            delete dbPool;
            dbPool = nullptr;
        }

        flushLog();
    }

    bool Server::isShutdown() const {
        return !running;
    }

    bool Server::isRunning() const {
        return running;
    }

    mongocxx::pool::entry Server::acquireDbClient() {
        return dbPool->acquire();
    }

    void Server::initDatabase() {
        assert(dbPool == nullptr);
        
        auto& c = launchConfig.database;
        SPDLOG_INFO("init a database pool, uri: {}", c.uri);
        mongocxx::uri uri{c.uri};
        dbPool = new mongocxx::pool(uri);
    }

}