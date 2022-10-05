#include "log.h"
#include "player.h"
#include "common.h"

#include "server.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <chrono>

#include "gameMsg.pb.h"
#include "command.pb.h"

#include <google/protobuf/text_format.h>

#include <absl/container/flat_hash_map.h>

namespace cdf {

    using namespace msg;

    absl::flat_hash_map<int, std::function<void(Player* player, GameMsgReq const& msg)>> handlerMap;


    namespace {

        void playerLogicThread(PlayerThreadLocal* threadLocal)  {
            using namespace std::chrono_literals;

            threadLocal->init();

            google::protobuf::TextFormat::Printer printer;
            printer.SetSingleLineMode(true);

            auto server = currentServer();
            while (server->isRunning()) {
                // 
                if(threadLocal->empty() || !threadLocal->hasMessage()) {
                    // queue->wait();
                    std::this_thread::sleep_for(1ms);
                    continue;
                }

                auto list = threadLocal->getListCopy();
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

    uint Player::getPlayerId() const {
        return this->playerId;
    }

    void Player::write(int cmd, int errorCode, google::protobuf::Message* message) {
        session->write(cmd, errorCode, message);
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

    PlayerThreadLocal::PlayerThreadLocal(mongocxx::pool::entry entry)
        : dbClientEntry(std::move(entry))
    {
        
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

    void PlayerThreadLocal::init() {
        // init a v8 isolate .
        

        std::string_view dbName = currentServer()->launchConfig.database.db;
        SPDLOG_INFO("use database {}", dbName);
        database = dbClientEntry->database(dbName);
    }

    mongocxx::database& PlayerThreadLocal::getDatabase() {
        return database;
    }

    void onCommand(int cmd, std::function<void(Player* player, msg::GameMsgReq const& msg)> callback) {
        assert(!handlerMap.contains(cmd));
        handlerMap[cmd] = callback;

#ifdef CyanDragonFishDebug
        SPDLOG_LOGGER_DEBUG(logConsole, "bind command handler {}", cmd);
#endif // DEBUG
    }

}