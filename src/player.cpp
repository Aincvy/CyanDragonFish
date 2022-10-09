#include "log.h"
#include "player.h"
#include "common.h"

#include "server.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <chrono>

#include "js_helper.h"

#include <google/protobuf/text_format.h>

#include <absl/container/flat_hash_map.h>
#include <vector>

namespace cdf {

    using namespace msg;

    absl::flat_hash_map<int, std::function<void(Player* player, GameMsgReq const& msg)>> handlerMap;


    namespace {

        void playerLogicThread(PlayerThreadLocal* threadLocal)  {
            using namespace std::chrono_literals;

            threadLocal->init();
            auto server = currentServer();
            
            // v8 things.
            auto isolate = threadLocal->getIsolate();
            v8::HandleScope handleScope(isolate);
            auto context = v8::Context::New(isolate);
            v8::Context::Scope contextScope(context);
            registerUtilFunctions(isolate, threadLocal);
            loadJsFiles(isolate, server->launchConfig.jsPath("lib/"));
            registerDomain(isolate);
            registerDatabaseOpr(isolate);
            loadJsFileInGlobal(isolate, server->launchConfig.jsPath("prelib.js"));
            registerDatabaseObject(isolate, threadLocal);
            loadJsFile(isolate, server->launchConfig.jsPath("postlib.js"));
            loadJsFiles(isolate, server->launchConfig.path.js);

            google::protobuf::TextFormat::Printer printer;
            printer.SetSingleLineMode(true);

            while (server->isRunning()) {
                // 
                if(threadLocal->empty() || !threadLocal->hasMessage()) {
                    // queue->wait();
                    std::this_thread::sleep_for(1ms);
                    continue;
                }

                auto& list = threadLocal->getList();
                for (auto item : list) {
                    auto session = item->getSession();
                    int tmpMsgCount = 0;
                    while (session->hasMessage() && ++tmpMsgCount < 15)
                    {
                        auto msg = session->nextMessage();
                    
                        // handle message .
                        auto handlerIter = handlerMap.find(msg.command());
                        if(handlerIter == handlerMap.end()) {
                            SPDLOG_LOGGER_ERROR(logMessage, "could not find handler for command: {}", msg.command());
                        } else {
                            handlerIter->second(item, msg);
                        }

                    }    
                }

                // std::this_thread::sleep_for(1ms);

            }
        }
    }

    Player::Player(NetworkSession* session)
        : session(session)
    {
        
    }

    Player::~Player()
    {
        
    }

    NetworkSession* Player::getSession() const {
        return this->session;
    }

    void Player::setSession(NetworkSession* session) {
        this->session = session;
    }

    uint Player::getPlayerId() const {
        return this->playerId;
    }

    void Player::write(int cmd, int errorCode, google::protobuf::Message* message) {
        if (session) {
            session->write(cmd, errorCode, message);    
        }
    }

    void Player::setPlayerThread(PlayerThreadLocal* threadLocal) {
        this->threadLocal = threadLocal;
    }

    PlayerThreadLocal* Player::getPlayerThread() {
        return threadLocal;
    }

    void PlayerManager::init() {
        assert(!initFlag);

        initFlag = true;
        auto server = currentServer();
        auto threadCount = std::max(server->launchConfig.server.logicThreadCount, (ushort)1);

        for (int i = 0; i < threadCount; i++) {
            auto p = new PlayerThreadLocal(server->acquireDbClient());
            queueMap[i] = p;

            auto t = new std::thread(playerLogicThread, p);
            logicThreads.push_back(t);
        }
    }

    void PlayerManager::addPlayer(Player* p) {
        assert(initFlag);
        auto index = p->getSession()->getSessionId() % queueMap.size();
        auto q = queueMap.at(index);
        q->addPlayer(p);
    }

    int PlayerManager::createPlayer(NetworkSession* session) {
        auto p = new Player(session);
        addPlayer(p);
        return CODE_OK;
    }

    void PlayerManager::destroy() {
        logConsole->info("start destroy playerManager");

        logConsole->info("wait for logic threads shutdown");
        for (auto& item : logicThreads) {
            item->join();
            delete item;
        }
        logicThreads.clear();

        logConsole->info("playerManager destroy success.");
    }

    void PlayerManager::removePlayer(NetworkSession* session) {
        auto playerId = session->getPlayerId();
        if (playerId > 0) {
            // lock ?
            playerMap.erase(playerId);
        }

        auto index = session->getSessionId() % queueMap.size();
        queueMap.at(index)->removePlayer(session);
    }

    PlayerThreadLocal::PlayerThreadLocal(mongocxx::pool::entry entry)
        : dbClientEntry(std::move(entry))
    {
        
    }

    PlayerThreadLocal::~PlayerThreadLocal()
    {
        serviceFuncMap.clear();
        isolateScope = nullptr;
        if (isolate ) {
            // delete isolate;
            isolate->Dispose();
            isolate = nullptr;
        }

    }

    bool PlayerThreadLocal::empty() const {
        return list.empty();
    }

    bool PlayerThreadLocal::hasMessage() const {
        bool flag = false;
        for (auto const& item : list) {
            if (item->getSession()->hasMessage())
            {
                flag = true;
                break;
            }
        }

        return flag;
    }

    void PlayerThreadLocal::addPlayer(Player* p) {
        std::lock_guard<std::mutex> l(this->listMutex);
        list.push_back(p);
        p->setPlayerThread(this);
    }

    void PlayerThreadLocal::removePlayer(NetworkSession* session) {
        for (auto it = list.begin(); it != list.end(); it++) {
            auto player = *it;
            if (player->getSession() == session)
            {
                player->setSession(nullptr);
                list.erase(it);
                break;
            }
        }
    }

    void PlayerThreadLocal::wait() {
        std::unique_lock<std::mutex> l(this->listMutex);
        condVar.wait(l);
    }

    void PlayerThreadLocal::notify() {
        condVar.notify_one();
    }

    std::vector<Player*> PlayerThreadLocal::getListCopy() const {
        return this->list;
    }

    std::vector<Player*>& PlayerThreadLocal::getList() {
        return this->list;
    }

    std::mutex& PlayerThreadLocal::getListMutex() {
        return listMutex;
    }

    void PlayerThreadLocal::init() {
        std::string_view dbName = currentServer()->launchConfig.database.db;
        SPDLOG_INFO("use database {}", dbName);
        database = dbClientEntry->database(dbName);

        // init a v8 isolate .
        v8::Isolate::CreateParams createParams;
        createParams.array_buffer_allocator =
                v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        this->isolate = v8::Isolate::New(createParams);
        this->isolateScope = std::make_unique<v8::Isolate::Scope>(this->isolate);
    }

    mongocxx::database& PlayerThreadLocal::getDatabase() {
        return database;
    }

    v8::Isolate* PlayerThreadLocal::getIsolate() {
        return isolate;
    }

    absl::flat_hash_map<std::string, V8Function>&  PlayerThreadLocal::getServiceFuncMap() {
        return serviceFuncMap;
    }

    void onCommand(int cmd, std::function<void(Player* player, msg::GameMsgReq const& msg)> callback) {
        assert(!handlerMap.contains(cmd));
        handlerMap[cmd] = callback;

#ifdef CyanDragonFishDebug
        SPDLOG_LOGGER_DEBUG(logConsole, "bind command handler {}", cmd);
#endif // DEBUG
    }

}