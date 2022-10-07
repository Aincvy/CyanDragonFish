/**
 * created by aincvy(aincvy@gmail.com) on 2022/10/5
 */

#pragma once

#include <string>

#include <mongocxx/pool.hpp>

namespace cdf {

    struct Entity {
        std::string _id;
        long createTime;
        // last update time
        long updateTime;  
    };

    struct Account: Entity {
        std::string id;
        std::string username;
        // sha256
        std::string password;
        std::string salt;
        long lastLoginTime;
    };

    struct Role: Entity {
        std::string accountId;
        uint playerId = 0; 
        std::string nickname = "";
        uint level = 0;
        uint exp = 0;
    };

    struct Item : Entity {
        uint playerId = 0;
        uint itemId = 0;
        uint count = 0;
    };

    void initPlayerId(mongocxx::pool::entry entry);

    uint nextPlayerId();

}