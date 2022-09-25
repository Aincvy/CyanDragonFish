/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <mutex>
#include <sys/types.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include <absl/container/flat_hash_map.h>

#define BUFFER_SIZE  512 * 1024

namespace cdf {
    
    struct NetMessage{
        uint command = 0;
        uint errorCode = 0;
        char *buffer = nullptr;
        uint bufferLength = 0;
    };

    struct NetworkSession {

        NetworkSession();
        NetworkSession(struct bufferevent *bev, int fd);
        ~NetworkSession();

        void write(const char* msg, ushort len);

        void close();

    private: 
        // 
        uint writeCursor = 0;
        char* readBuffer[BUFFER_SIZE] = {0};
        char* writeBuffer[BUFFER_SIZE] = {0};

        int fd = 0;
        struct bufferevent *bev = nullptr;

        void reset();
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

        bool addNewSession(struct bufferevent *bev, int fd);

    private:
        struct event_base *base;
	    struct evconnlistener *listener;
	    struct event *signal_event;
        struct sockaddr_in sin;

        absl::flat_hash_map<struct bufferevent *, NetworkSession> sessionMap;
        std::mutex sessionMapMutex;    // mutex for session map.

    };

}