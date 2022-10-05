/**
 * created by aincvy(aincvy@gmail.com) on 2022/10/5
 */

#pragma once

#include <string>

#include <mongocxx/pool.hpp>

namespace cdf {

    struct Account {
        std::string id;
        std::string username;
        // sha256
        std::string password;
        std::string salt;
        long createTime;
        long lastLoginTime;
    };

    struct Role {
        uint playerId = 0; 
        std::string nickname = "";
        uint level = 0;
        uint exp = 0;
    };

    struct Item {
        uint playerId = 0;
        uint itemId = 0;
        uint count = 0;
    };

    void initPlayerId(mongocxx::pool::entry entry);

    uint nextPlayerId();

}