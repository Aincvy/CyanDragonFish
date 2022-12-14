#include "log.h"

#include "network.h"

#include "common.h"
#include "server.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <arpa/inet.h>

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/text_format.h>
#include <mutex>
#include <netinet/in.h>
#include <shared_mutex>
#include <signal.h>

#include <sstream>
#include <string>
#include <sys/types.h>
#include <thread>
#include <chrono>
#include <iostream>

#include <absl/random/random.h>
#include <absl/strings/substitute.h>

#include "util.h"

namespace cdf {
    
    using namespace msg;

    namespace {
        void networkWriteThreadRun() {
            using namespace std::chrono_literals;

            auto server = currentServer();
            while (server->isRunning()) {
                auto now = currentTime(); 
                {
                    // std::shared_lock<std::shared_mutex> l(server->networkManager.getSessionMapMutex());
                    auto map = server->networkManager.getSessionMap();
                    // write buffer to client
                    for(auto & item: map){
                        auto session = item.second;
                        if (session->isClosed())
                        {
                            // 
                            if (now - session->getCloseTime() > 5 * 2 * 1000) {
                                // remove 
                                server->networkManager.removeSession(session);
                            }

                            continue;
                        }
                        if(!session->isWriteAble()) {
                            continue;
                        }

                        auto buf = session->getWriteBuffer();
                        BufferLockGuard l(buf);

                        auto len = evbuffer_get_length(buf);
                        if (len > 0) {
                            logMessage->debug("write bytes({}) to {}", len, session->debugString());
                            evbuffer_write(buf, session->getFd());
                        }
                    }
                }
                
                std::this_thread::sleep_for(1ms);
            }
        }
    }

    static void read_cb(struct bufferevent *bev, void *arg)
    {
        auto* session = (NetworkSession*) arg;
        struct evbuffer *in = bufferevent_get_input(bev);
        
        auto len = evbuffer_get_length(in);
        if(len > 0){
            auto target = session->getReadBuffer();
            BufferLockGuard l(target);
            auto r = evbuffer_remove_buffer(in, target, len);
            assert(r != 0);
        }
    }

    static void conn_writecb(struct bufferevent *bev, void *user_data)
    {
        auto* session = (NetworkSession*) user_data;
        session->setWriteAble(true);

        // logMessage->debug("conn_writecb...");
        // struct evbuffer *output = bufferevent_get_output(bev);
        // if (evbuffer_get_length(output) == 0) {
        //     printf("flushed answer\n");
        //     bufferevent_free(bev);
        // }
    }
    
    static void fd_eventcb(struct bufferevent *bev, short events, void *user_data)
    {
        auto* session = (NetworkSession*) user_data;
        session->setWriteAble(false);
        if (events & BEV_EVENT_EOF) {
            logMessage->info("session closed {}, playerId: {}, remote address: {}:{}", session->getSessionId(), session->getPlayerId(), session->getIpAddress(), session->getPort());
        } else if (events & BEV_EVENT_ERROR) {
            SPDLOG_LOGGER_ERROR(logError, "Got an error on the connection. {}, playerId: {}, remote address: {}:{}, reason: {}", 
                session->getSessionId(), session->getPlayerId(),  session->getIpAddress(), session->getPort(), strerror(errno) );
        }
        /* None of the other events can happen here, since we haven't enabled
        * timeouts */
        session->close();
        session->freeConnection();
    }

    static void listener_cb(struct evconnlistener *, evutil_socket_t fd,
        struct sockaddr *sa, int socklen, void *user_data){
        struct event_base *base = (struct event_base *)user_data;
        struct bufferevent *bev;

        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        if (!bev) {
            logError->error("Error constructing bufferevent!");
            // event_base_loopbreak(base);
            return;
        }
        
        SPDLOG_DEBUG("new connection coming.");
        auto networkMgr =  &(currentServer()->networkManager);
        NetworkSession* session = nullptr;
        if((session = currentServer()->networkManager.addNewSession(bev, fd))){
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *sin = (struct sockaddr_in *)sa;
            inet_ntop (AF_INET, &sin->sin_addr, ip, sizeof (ip));
            session->setAddressInfo(ip, htons(sin->sin_port));
            logMessage->info("session created: {}, remote address: {}:{}", session->getSessionId(), session->getIpAddress(), session->getPort());
            session->open();

            currentServer()->playerManager.createPlayer(session);
            bufferevent_setcb(bev, read_cb, conn_writecb, fd_eventcb, session);
            bufferevent_enable(bev, EV_WRITE | EV_READ);
        } else {
            logError->error("add new session failed");
        }
    }

    static void signal_cb(evutil_socket_t, short, void *user_data){
        struct event_base *base = (struct event_base *)user_data;
        struct timeval delay = { 1, 0 };

        printf("Caught an interrupt signal; exiting cleanly in 1 seconds.\n");

        currentServer()->shutdownServer();
        event_base_loopexit(base, &delay);
    }

    void NetworkManager::serv() {
        assert(base);
        SPDLOG_LOGGER_INFO(logConsole, "network serv");
        event_base_dispatch(base);
    }

    void NetworkManager::init() {
        if(base){
            logError->error("NetworkManager cannot do init twice.");
            return;
        }

        SPDLOG_INFO("check protobuf version.");
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        SPDLOG_INFO("check success, protobuf version: {}, v{}", GOOGLE_PROTOBUF_VERSION, google::protobuf::internal::VersionString(GOOGLE_PROTOBUF_VERSION));

        // evthread_use_pthreads();      // ... 
        base = event_base_new();
        if(base == nullptr){
            logError->error("Could not initialize libevent!");
            exit(CODE_NETWORK_FAILED);
        }

        auto server = currentServer();
        auto const& config = server->launchConfig.server;
        sin.sin_family = AF_INET;
	    sin.sin_port = htons(config.port);
        sin.sin_addr = { inet_addr(config.address.c_str()) };

        SPDLOG_INFO("try bind to {}:{}", config.address, config.port);

        listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
	    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
	    (struct sockaddr*)&sin,sizeof(sin));
        if(!listener){
            logError->error("Could not create a listener!");
            exit(CODE_NETWORK_FAILED);
        }

        signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
        if (!signal_event || event_add(signal_event, NULL)<0) {
            logError->error("Could not create/add a signal event!\n");
            exit(CODE_NETWORK_FAILED);
        }

        SPDLOG_INFO("try start network-thread");
        networkWriteThread = new std::thread(networkWriteThreadRun);

        SPDLOG_INFO("networkManager init success.");
    }

    void NetworkManager::detroy() {
        SPDLOG_INFO("destroy networkManager...");

        SPDLOG_INFO("wait for network thread shutdown");
        networkWriteThread->join();
        delete networkWriteThread;
        networkWriteThread = nullptr;

        evconnlistener_free(listener);
        event_free(signal_event);
        event_base_free(base);

        listener = nullptr;
        signal_event = nullptr;
        base = nullptr;

        SPDLOG_INFO("destroy google protobuf ");
        google::protobuf::ShutdownProtobufLibrary();

        SPDLOG_INFO("destroy networkManager done.");
    }

    NetworkSession* NetworkManager::addNewSession(struct bufferevent *bev, int fd) {
        if(!bev) {
            return nullptr;
        }

        std::unique_lock<std::shared_mutex> guard(this->sessionMapMutex);
        
        auto s = new NetworkSession(bev, fd, ++ sessionIdIncr);
        sessionMap[bev] = s;
        return s;
    }

    bool NetworkManager::removeSession(struct bufferevent* bev) {
        auto result = sessionMap.find(bev);
        return removeSession(result);
    }

    bool NetworkManager::removeSession(NetworkSession* session) {
        auto result = std::find_if(sessionMap.begin(), sessionMap.end(), [session](auto const& item){
            return item.second == session;
        });
        
        return removeSession(result);
    }

    absl::flat_hash_map<struct bufferevent *, NetworkSession*>& NetworkManager::getSessionMap() {
        return this->sessionMap;
    }

    std::shared_mutex& NetworkManager::getSessionMapMutex() {
        return sessionMapMutex;
    }

    bool NetworkManager::removeSession(absl::flat_hash_map<struct bufferevent *, NetworkSession*>::iterator result) {
        if(result != sessionMap.end()){
            std::unique_lock<std::shared_mutex> l(this->sessionMapMutex);

            auto session = result->second;
            SPDLOG_LOGGER_INFO(logMessage, "remove session: {}", session->debugString());
            currentServer()->playerManager.removePlayer(session);
            delete session;
            sessionMap.erase(result);
            return true;
        }
        return false;
    }

    NetworkSession::NetworkSession(int sessionId) : sessionId(sessionId)
    {
        initBuffer();
        openFlag = true;
    }

    NetworkSession::NetworkSession(struct bufferevent *bev, int fd, int sessionId)
        : bev(bev), fd(fd), sessionId(sessionId)
    {
        initBuffer();
        openFlag = true;
        createTime = currentTime();
    }

    NetworkSession::~NetworkSession()
    {
        reset();
    }

    void NetworkSession::write(msg::GameMsgRes const& msg) {
        auto str = msg.SerializeAsString();    
        BufferLockGuard l(writeBuffer);
        ushort len = str.length();
        evbuffer_add(writeBuffer, &len, sizeof(len));
        evbuffer_add(writeBuffer, str.c_str(), str.length());
    }

    void NetworkSession::write(int cmd, int errorCode, google::protobuf::Message* message) {
        GameMsgRes msg;
        msg.set_command(cmd);
        msg.set_error_code(errorCode);
        if(message) {
            std::string str = message->SerializeAsString();
            msg.set_content(str);

            static google::protobuf::TextFormat::Printer printer;
            printer.SetSingleLineMode(true);
            if(std::string tmp; printer.PrintToString(*message, &tmp)){
                logMessage->debug("Send To {},[CEV]: {} {} [{}] {}", playerId, cmd, errorCode,message->GetTypeName(), tmp);
            } else {
                logMessage->debug("Send To {},[CEV]: {} {} [{}] {}", playerId, cmd, errorCode, message->GetTypeName(), message->ShortDebugString());
            }
        } else {
            logMessage->debug("Send To {},[CEV]: {} {}", playerId, cmd, errorCode);
        }
        if (!writeAble) {
            logMessage->debug("Session from playerId({}) is not writeable, msg maybe not arrived. ", playerId);
        }

        write(msg);
    }

    void NetworkSession::close(int reason) {
        SPDLOG_LOGGER_DEBUG(logMessage, "session close: {}", debugString());

        closeTime = currentTime();
        closedFlag = true;
        writeAble = false;
        openFlag = false;
    }

    void NetworkSession::open() {
        SPDLOG_LOGGER_DEBUG(logMessage, "session open, fd: {}", fd);

        closedFlag = false;
        openFlag = true;
        closeTime = 0;
    }

    void NetworkSession::setAddressInfo(std::string_view ip, ushort port) {
        this->ipAddress = ip;
        this->port = port;
    }

    std::string_view NetworkSession::getIpAddress() const {
        return this->ipAddress;   
    }

    ushort NetworkSession::getPort() const {
        return this->port;
    }

    struct evbuffer* NetworkSession::getReadBuffer() {
        return readBuffer;
    }

    struct evbuffer* NetworkSession::getWriteBuffer() {
        return writeBuffer;
    }

    int NetworkSession::getFd() const {
        return this->fd;
    }

    bool NetworkSession::hasMessage() {
        BufferLockGuard l(readBuffer);
        auto len = evbuffer_get_length(readBuffer);
        if(len <= 0) {
            return false;
        }

        if(currentPacketLength == 0) {
            // 2 byte as length
            constexpr const int lengthHolderLen = sizeof(currentPacketLength);
            if (len < lengthHolderLen)
            {
                return false;
            }

            evbuffer_remove(readBuffer, &currentPacketLength, lengthHolderLen);
            len -= lengthHolderLen;
        }

        return len >= currentPacketLength;
    }

    GameMsgReq NetworkSession::nextMessage() {
        BufferLockGuard l(readBuffer);
        auto len = evbuffer_get_length(readBuffer);
        if(len < currentPacketLength) {
            return {};
        }

        auto buffer = new char[currentPacketLength];
        evbuffer_remove(readBuffer, buffer, currentPacketLength);
        // std::stringstream ss;
        // for(int i = 0; i < currentPacketLength; i++) {
        //     ss << static_cast<int>(buffer[i]);
        //     ss << ", ";
        // }
        // SPDLOG_LOGGER_DEBUG(logMessage, "bytes: {}", ss.str());
        GameMsgReq msgReq;
        if(!msgReq.ParseFromArray(buffer, currentPacketLength)){
            SPDLOG_LOGGER_ERROR(logError, "parse game msg error from {}:{}", getIpAddress(), getPort());
        }
        delete [] buffer;
        currentPacketLength = 0;
        return msgReq;
    }

    int NetworkSession::getSessionId() const {
        return sessionId;
    }

    void NetworkSession::setPlayerId(uint playerId) {
        this->playerId = playerId;
    }

    uint NetworkSession::getPlayerId() const {
        return playerId;
    }

    std::string NetworkSession::debugString() const {
        return absl::Substitute("$0,playerId: $1, remote address: $2:$3", sessionId, playerId, ipAddress, port);
    }

    bool NetworkSession::isWriteAble() const {
        return writeAble;
    }

    void NetworkSession::setWriteAble(bool v) {
        writeAble = v;
    }

    void NetworkSession::freeConnection() {
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }

        fd = 0;
    }

    bool NetworkSession::isClosed() const {
        return closedFlag;
    }

    bool NetworkSession::isOpen() const {
        return openFlag;
    }

    long NetworkSession::getCloseTime() const {
        return closeTime;
    }

    long NetworkSession::getCreateTime() const {
        return createTime;
    }

    void NetworkSession::reset() {
        
        if(readBuffer) {
            evbuffer_free(readBuffer);
        }
        if(writeBuffer){
            evbuffer_free(writeBuffer);
        }
        readBuffer = nullptr;
        writeBuffer = nullptr;

        freeConnection();
    }

    void NetworkSession::initBuffer() {
        if(!readBuffer){
            readBuffer = evbuffer_new();
            evbuffer_enable_locking(readBuffer, nullptr);
        }
        
        if(!writeBuffer){
            writeBuffer = evbuffer_new();
            evbuffer_enable_locking(writeBuffer, nullptr);
        }
        
    }

    BufferLockGuard::BufferLockGuard(struct evbuffer* buf)
        : buf(buf)
    {
        evbuffer_lock(buf);
    }

    BufferLockGuard::~BufferLockGuard()
    {
        evbuffer_unlock(buf);
    }

}