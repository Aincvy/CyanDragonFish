#include "player.h"
#include "common.h"
#include "log.h"

#include "server.h"
#include <algorithm>
#include <cassert>
#include <mutex>
#include <sys/types.h>
#include <thread>
#include <chrono>

namespace cdf {

    namespace {

        void playerLogicThread(PlayerThreadQueue* queue)  {
            using namespace std::chrono_literals;

            auto server = currentServer();
            while (server->isRunning()) {
                // 
                if(queue->empty() || !queue->hasMessage()) {
                    queue->wait();
                }

                auto list = queue->getListCopy();
                for (auto item : list) {
                    auto session = item->getSession();
                    int tmpMsgCount = 0;
                    while (session->hasMessage() && ++tmpMsgCount < 15)
                    {
                        auto msg = session->nextMessage();
                        logMessage->debug("Receive Message: {}", msg.command);
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

    void PlayerManager::init() {
        assert(!initFlag);

        initFlag = true;
        auto server = currentServer();
        auto threadCount = std::max(server->launchConfig.server.logicThreadCount, (ushort)1);

        for (int i = 0; i < threadCount; i++) {
            auto p = new PlayerThreadQueue();
            queueMap[i] = p;

            std::thread thread(playerLogicThread, p);
        }
    }

    void PlayerManager::addPlayer(Player* p) {
        assert(initFlag);
        auto index = p->getSession()->getSessionId() % queueMap.size();
        queueMap.at(index)->addPlayer(p);
    }

    int PlayerManager::createPlayer(NetworkSession* session) {
        auto p = new Player(session);
        addPlayer(p);
        return CODE_OK;
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