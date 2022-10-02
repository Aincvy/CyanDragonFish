/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <sys/types.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "network.h"

#include <absl/container/flat_hash_map.h>

namespace cdf {

    struct PlayerThreadQueue;

    /**
     * player class 
     */
    struct Player {
        Player(NetworkSession* session);
        ~Player();

        void login();

        NetworkSession* getSession() const;

        uint getPlayerId() const;

        void write(int cmd, int errorCode, google::protobuf::Message* message = nullptr);

        void setPlayerThreadQueue(PlayerThreadQueue* queue);

    private:
        uint playerId = 0;
        NetworkSession* session = nullptr;
        // login or not ?
        bool isLogin = false;

        PlayerThreadQueue* threadQueue = nullptr;
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

        void destroy();

    private:
        absl::flat_hash_map<uint, Player*> playerMap;
        absl::flat_hash_map<uint, PlayerThreadQueue*> queueMap;

        std::vector<std::thread*> logicThreads;

        bool initFlag = false;
    };

}