/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <sys/types.h>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "network.h"

#include <absl/container/flat_hash_map.h>

namespace cdf {

    /**
     * player class 
     */
    struct Player {
        Player(NetworkSession* session);
        ~Player();

        void login();

        NetworkSession* getSession() const;

        uint getPlayerId() const;



    private:
        uint playerId = 0;
        NetworkSession* session = nullptr;
        // login or not ?
        bool isLogin = false;

    };

    /**
     * Not a queue. 
     */
    struct PlayerThreadQueue {
        
        bool empty() const;

        bool hasMessage() const;

        void addPlayer(Player* p);

        void wait();

        void notify();

        std::vector<Player*> getListCopy() const;    

    private:
        std::vector<Player*> list;
        std::mutex listMutex; 
        std::condition_variable condVar;
    };

    struct PlayerManager {
        
        void init();

        void addPlayer(Player* p);

        int createPlayer(NetworkSession* session);

    private:
        absl::flat_hash_map<uint, Player*> playerMap;
        absl::flat_hash_map<uint, PlayerThreadQueue*> queueMap;

        bool initFlag = false;
    };

}