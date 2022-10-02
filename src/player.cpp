#include "log.h"
#include "player.h"
#include "common.h"

#include "server.h"
#include <algorithm>
#include <cassert>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <thread>
#include <chrono>

#include "gameMsg.pb.h"
#include "command.pb.h"

#include <google/protobuf/text_format.h>

namespace cdf {

    using namespace msg;

    namespace {

        void playerLogicThread(PlayerThreadQueue* queue)  {
            using namespace std::chrono_literals;

            google::protobuf::TextFormat::Printer printer;
            printer.SetSingleLineMode(true);

            auto server = currentServer();
            while (server->isRunning()) {
                // 
                if(queue->empty() || !queue->hasMessage()) {
                    // queue->wait();
                    std::this_thread::sleep_for(1ms);
                    continue;
                }

                auto list = queue->getListCopy();
                for (auto item : list) {
                    auto session = item->getSession();
                    int tmpMsgCount = 0;
                    while (session->hasMessage() && ++tmpMsgCount < 15)
                    {
                        auto msg = session->nextMessage();
                        
                        
                        
                        // handle message .
                        ReqHello reqHello;
                        if(reqHello.ParseFromString(msg.content())) {

                            if(std::string tmp; printer.PrintToString(reqHello, &tmp)){
                                logMessage->debug("Recv Fm {},[CV] {} [{}] {}", item->getPlayerId(), msg.command(),reqHello.GetTypeName(), tmp);
                            } else {
                                logMessage->debug("Recv Fm {},[CV] {} [{}] {}", item->getPlayerId(), msg.command(),reqHello.GetTypeName(), reqHello.ShortDebugString());
                            }

                            // write 
                            ResHello resHello;
                            resHello.set_is_full(false);
                            resHello.set_server_version(SERVER_VERSION_STR);

                            item->write(Command::Hello, 0, &resHello);
                        } else {
                            logConsole->debug("parse reqHello, error.");
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

    void Player::setPlayerThreadQueue(PlayerThreadQueue* queue) {
        this->threadQueue = queue;
    }

    void PlayerManager::init() {
        assert(!initFlag);

        initFlag = true;
        auto server = currentServer();
        auto threadCount = std::max(server->launchConfig.server.logicThreadCount, (ushort)1);

        for (int i = 0; i < threadCount; i++) {
            auto p = new PlayerThreadQueue();
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

    bool PlayerThreadQueue::empty() const {
        return list.empty();
    }

    bool PlayerThreadQueue::hasMessage() const {
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

    void PlayerThreadQueue::addPlayer(Player* p) {
        std::lock_guard<std::mutex> l(this->listMutex);
        list.push_back(p);
        p->setPlayerThreadQueue(this);
    }

    void PlayerThreadQueue::wait() {
        std::unique_lock<std::mutex> l(this->listMutex);
        condVar.wait(l);
    }

    void PlayerThreadQueue::notify() {
        condVar.notify_one();
    }

    std::vector<Player*> PlayerThreadQueue::getListCopy() const {
        return this->list;
    }

}