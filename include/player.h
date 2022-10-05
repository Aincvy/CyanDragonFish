/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <functional>
#include <sys/types.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "network.h"

#include <absl/container/flat_hash_map.h>

#include "gameMsg.pb.h"

#include <v8.h>

#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/pool.hpp>


namespace cdf {

    struct PlayerThreadLocal;

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

        void setPlayerThread(PlayerThreadLocal* queue);

        PlayerThreadLocal* getPlayerThread();

    private:
        uint playerId = 0;
        NetworkSession* session = nullptr;
        // login or not ?
        bool isLogin = false;

        PlayerThreadLocal* threadLocal = nullptr;
    };

    /**
     * 
     */
    struct PlayerThreadLocal {
        
        PlayerThreadLocal(mongocxx::pool::entry entry);

        bool empty() const;

        bool hasMessage() const;

        void addPlayer(Player* p);

        void wait();

        void notify();

        std::vector<Player*> getListCopy() const;    

        void init();

        mongocxx::database& getDatabase();

    private:
        std::vector<Player*> list;
        std::mutex listMutex; 
        std::condition_variable condVar;
        v8::Isolate* isolate = nullptr;

        mongocxx::pool::entry dbClientEntry;
        mongocxx::database database;
    };

    struct PlayerManager {
        
        void init();

        void addPlayer(Player* p);

        int createPlayer(NetworkSession* session);

        void destroy();

    private:
        absl::flat_hash_map<uint, Player*> playerMap;
        absl::flat_hash_map<uint, PlayerThreadLocal*> queueMap;

        std::vector<std::thread*> logicThreads;

        bool initFlag = false;
    };

    void onCommand(int cmd, std::function<void(Player* player, msg::GameMsgReq const& msg)> callback);

}