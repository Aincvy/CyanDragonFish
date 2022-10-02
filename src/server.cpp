#include "log.h"

#include "server.h"
#include "common.h"
#include "config.h"

namespace cdf {

    static Server* g_server = nullptr;

    int initServer(std::string_view configPath) {
        g_server = new Server();
        if(!configPath.empty()){
            loadConfig(configPath);
        }
        
        // init 
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

        flushLog();
    }

    bool Server::isShutdown() const {
        return !running;
    }

    bool Server::isRunning() const {
        return running;
    }

}