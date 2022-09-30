/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <sys/types.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include <absl/container/flat_hash_map.h>

#include <atomic>

#define BUFFER_SIZE  512 * 1024

namespace cdf {
    
    struct BufferLockGuard{
        BufferLockGuard(struct evbuffer* buf);
        ~BufferLockGuard();

    private:
        struct evbuffer* buf = nullptr;
    };

    struct NetMessage{
        uint command = 0;
        uint errorCode = 0;
        char *buffer = nullptr;
        uint bufferLength = 0;

        /**
         * free buffer 
         */
        void free();

    };

    struct NetworkSession {

        NetworkSession(int sessionId);
        NetworkSession(struct bufferevent *bev, int fd, int sessionId);
        ~NetworkSession();

        void write(const char* msg, ushort len);

        void close();

        void setAddressInfo(std::string_view ip, ushort port);

        std::string_view getIpAddress() const;
        ushort getPort() const;

        struct evbuffer* getReadBuffer();
        struct evbuffer* getWriteBuffer();

        int getFd() const;

        /**
         * 
         */
        bool hasMessage();

        /**
         * after use the msg, you need call `free` function.
         */
        NetMessage nextMessage();

        int getSessionId() const;        

    private: 
        // 
        // uint writeCursor = 0;
        // char* readBuffer[BUFFER_SIZE] = {0};
        // char* writeBuffer[BUFFER_SIZE] = {0};

        struct evbuffer* readBuffer = nullptr;
        struct evbuffer* writeBuffer = nullptr;

        int fd = 0;
        struct bufferevent *bev = nullptr;

        std::string ipAddress;
        ushort port;

        ushort currentPacketLength = 0;

        int sessionId = 0;

        void reset();
        void initBuffer(); 
    };


    struct NetworkManager {

        /**
         * 
         */
        void serv();

        /**
         * 
         */
        void init();

        void detroy();

        /**
         * Create a session.
         */
        NetworkSession* addNewSession(struct bufferevent *bev, int fd);

        /**
         * Remove a session.
         */
        bool removeSession(struct bufferevent* bev);

        /**
         * 
         */
        void onSessionClosed(struct  bufferevent* bev, int reason = 0);

        absl::flat_hash_map<struct bufferevent *, NetworkSession*> getSessionMapCopy();

    private:
        struct event_base *base;
	    struct evconnlistener *listener;
	    struct event *signal_event;
        struct sockaddr_in sin;

        absl::flat_hash_map<struct bufferevent *, NetworkSession*> sessionMap;
        std::mutex sessionMapMutex;    // mutex for session map.
        std::atomic_int sessionIdIncr = 10000;
    };

}