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
        
        // init net work manager
        g_server->networkManager.init();
        g_server->networkManager.serv();
        return CODE_OK;
    }

    Server* currentServer() {
        return g_server;
    }

}