/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include "log.h"

#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "js_helper.h"
#include "network.h"

#include <absl/container/flat_hash_map.h>

#include "gameMsg.pb.h"
#include "v8pp/convert.hpp"

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
        void setSession(NetworkSession* session);

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
        ~PlayerThreadLocal();

        bool empty() const;

        bool hasMessage() const;

        void addPlayer(Player* p);

        void removePlayer(NetworkSession* session);

        void wait();

        void notify();

        std::vector<Player*> getListCopy() const;    

        std::vector<Player*>& getList();
        std::mutex& getListMutex();

        void init();

        mongocxx::database& getDatabase();

        v8::Isolate* getIsolate();

        absl::flat_hash_map<std::string, V8Function>&  getServiceFuncMap();

    private:
        std::vector<Player*> list;
        std::mutex listMutex; 
        std::condition_variable condVar;
        v8::Isolate* isolate = nullptr;
        std::unique_ptr<v8::Isolate::Scope> isolateScope = nullptr;

        mongocxx::pool::entry dbClientEntry;
        mongocxx::database database;

        absl::flat_hash_map<std::string, V8Function> serviceFuncMap;
    };

    struct PlayerManager {
        
        void init();

        void addPlayer(Player* p);

        int createPlayer(NetworkSession* session);

        void destroy();

        /**
         * remove player from session.  delete memory. 
         */
        void removePlayer(NetworkSession* session);

    private:
        absl::flat_hash_map<uint, Player*> playerMap;
        // key sessionId-related.
        absl::flat_hash_map<uint, PlayerThreadLocal*> queueMap;
        
        std::vector<std::thread*> logicThreads;

        bool initFlag = false;
    };

    void onCommand(int cmd, std::function<void(Player* player, msg::GameMsgReq const& msg)> callback);

    

    /**
     * 
     */
    template<typename... Args>
    v8::Local<v8::Value> execServiceFunc(Player *player,std::string_view funcName, 
            std::function<void(Player*, int)> onErrorCode, Args&&... args) {
        // find function.
        PlayerThreadLocal* threadLocal = player->getPlayerThread();
        auto funcIter = threadLocal->getServiceFuncMap().find(funcName);
        if (funcIter == threadLocal->getServiceFuncMap().end()) {
            SPDLOG_LOGGER_ERROR(logError, "service function {} not found, ignore it", funcName);
            return {};
        }

        v8::Isolate* isolate = threadLocal->getIsolate();
        v8::HandleScope handleScope(isolate);
        // auto context =  isolate->GetCurrentContext();
        auto recv = v8::Object::New(isolate);
        auto func =  funcIter->second.Get(isolate);
        v8::Local<v8::Value> result;
        auto playerV8Object  = v8::Object::New(isolate);     // todo 
        result = v8pp::call_v8(isolate, func, recv, playerV8Object,std::forward<Args>(args)...);
        if (result.IsEmpty()) {
            return {};
        }

        if (result->IsNumber() || result->IsNumberObject()) {
            // error code
            auto errorCode = v8pp::from_v8<int>(isolate, result);
            onErrorCode(player, errorCode);
            return {};
        } else {
            return result;
        }
    }


}