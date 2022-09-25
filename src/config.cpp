#include "config.h"
#include "common.h"
#include "server.h"

#include "yaml-cpp/yaml.h"
#include <string>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/parser.h>

#include <fstream>

#include "log.h"
#include "spdlog/spdlog.h"

namespace cdf {

    int loadConfig(std::string_view path) {
        auto& config = currentServer()->launchConfig;
        std::ifstream fin(path.data());
        if(!fin.is_open()){
            logError->error("config path {} is invalid.", path);
            return CODE_CONFIG_FILE_ERROR;
        }

        auto root = YAML::Load(fin);
        auto serverNode = root["server"];
        if(serverNode.IsDefined()){
            auto temp = serverNode["port"];
            if(temp.IsDefined()){
                config.server.port = temp.as<uint>();
            }

            temp = serverNode["address"];
            if (temp.IsDefined())
            {
                config.server.address = temp.as<std::string>();
            }
        }

        auto  playerNode = root["player"];
        if (playerNode.IsDefined())
        {
            auto temp = playerNode["maxOnline"];
            if(temp.IsDefined()){
                config.player.maxOnline = temp.as<uint>();
            }
        }
        logConsole->info("read config file {} success.", path);
        return CODE_OK;
    }

}