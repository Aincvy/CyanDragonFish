#include "network.h"

#include "common.h"
#include "server.h"
#include "spdlog/spdlog.h"
#include "log.h"
#include <cassert>
#include <cstdlib>
#include <arpa/inet.h>

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <mutex>
#include <netinet/in.h>
#include <signal.h>

#include <sys/types.h>
#include <thread>
#include <chrono>
#include <iostream>

#include <absl/random/random.h>

namespace cdf {
    
    namespace {
        void networkWriteThread() {
            using namespace std::chrono_literals;

            auto server = currentServer();
            while (server->isRunning()) {
                auto map = server->networkManager.getSessionMapCopy();
                // write buffer to client
                for(auto & item: map){
                    auto session = item.second;
                    auto buf = session->getWriteBuffer();
                    BufferLockGuard l(buf);

                    evbuffer_write(buf, session->getFd());
                }

                std::this_thread::sleep_for(1ms);
            }
        }
    }

    static void read_cb(struct bufferevent *bev, void *arg)
    {
        auto* session = (NetworkSession*) arg;
        struct evbuffer *in = bufferevent_get_input(bev);
        // evbuffer_write(in, fileno(ctx->fout));
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
        // struct evbuffer *output = bufferevent_get_output(bev);
        // if (evbuffer_get_length(output) == 0) {
        //     printf("flushed answer\n");
        //     bufferevent_free(bev);
        // }
    }
    
    static void fd_eventcb(struct bufferevent *bev, short events, void *user_data)
    {
        auto* session = (NetworkSession*) user_data;
        if (events & BEV_EVENT_EOF) {
            // printf("Connection closed.\n");
            logMessage->info("session closed, remote address: {}:{}", session->getIpAddress(), session->getPort());
        } else if (events & BEV_EVENT_ERROR) {
            printf("Got an error on the connection: %s\n",
                strerror(errno));/*XXX win32*/
        }
        /* None of the other events can happen here, since we haven't enabled
        * timeouts */
        bufferevent_free(bev);
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
        
        auto networkMgr =  &(currentServer()->networkManager);
        NetworkSession* session = nullptr;
        if((session = currentServer()->networkManager.addNewSession(bev, fd))){
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in *sin = (struct sockaddr_in *)sa;
            inet_ntop (AF_INET, &sin->sin_addr, ip, sizeof (ip));
            session->setAddressInfo(ip, htons(sin->sin_port));
            logMessage->info("session created, remote address: {}:{}", session->getIpAddress(), session->getPort());

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
        logConsole->info("network serv");
        event_base_dispatch(base);
    }

    void NetworkManager::init() {
        if(base){
            logError->error("NetworkManager cannot do init twice.");
            return;
        }

        evthread_use_pthreads();
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

        logConsole->info("try bind to {}:{}", config.address, config.port);

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

        logConsole->info("try start network-thread");
        std::thread t(networkWriteThread);

        logConsole->info("networkManager init success.");
    }

    void NetworkManager::detroy() {
        logConsole->info("destroy networkManager...");
        evconnlistener_free(listener);
        event_free(signal_event);
        event_base_free(base);

        listener = nullptr;
        signal_event = nullptr;
        base = nullptr;
        logConsole->info("destroy networkManager done.");
    }

    NetworkSession* NetworkManager::addNewSession(struct bufferevent *bev, int fd) {
        if(!bev) {
            return nullptr;
        }

        // std::lock_guard<std::mutex> guard(this->sessionMapMutex);
        
        auto s = new NetworkSession(bev, fd, ++ sessionIdIncr);
        sessionMap[bev] = s;
        return s;
    }

    bool NetworkManager::removeSession(struct bufferevent* bev) {
        auto result = sessionMap.find(bev);
        if(result != sessionMap.end()){

            delete result->second;
            sessionMap.erase(result);
            return true;
        }
        return false;
    }

    void NetworkManager::onSessionClosed(struct  bufferevent* bev, int reason) {
        removeSession(bev);
    }

    absl::flat_hash_map<struct bufferevent *, NetworkSession*> NetworkManager::getSessionMapCopy() {
        return this->sessionMap;
    }

    NetworkSession::NetworkSession(int sessionId) : sessionId(sessionId)
    {
        initBuffer();
    }

    NetworkSession::NetworkSession(struct bufferevent *bev, int fd, int sessionId)
        : bev(bev), fd(fd), sessionId(sessionId)
    {
        initBuffer();
    }

    NetworkSession::~NetworkSession()
    {
        reset();
    }

    void NetworkSession::close() {
        reset();
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

    NetMessage NetworkSession::nextMessage() {
        BufferLockGuard l(readBuffer);
        auto len = evbuffer_get_length(readBuffer);
        if(len < currentPacketLength) {
            return {};
        }

        NetMessage msg;
        // read command 
        evbuffer_remove(readBuffer, &msg.command, sizeof(msg.command));
        currentPacketLength -= sizeof(msg.command);
        // read body 
        msg.bufferLength = currentPacketLength;
        msg.buffer = (char*)calloc(1, sizeof(char) * currentPacketLength);
        evbuffer_remove(readBuffer, msg.buffer, msg.bufferLength);
        return msg;
    }

    int NetworkSession::getSessionId() const {
        return sessionId;
    }

    void NetworkSession::reset() {
        // todo 
        if(readBuffer) {
            evbuffer_free(readBuffer);
        }
        if(writeBuffer){
            evbuffer_free(writeBuffer);
        }
        readBuffer = nullptr;
        writeBuffer = nullptr;
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

    void NetMessage::free() {
        if(this->buffer){
            std::free(buffer);
            buffer = nullptr;
        }
    }

}