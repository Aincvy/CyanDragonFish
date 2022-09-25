/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <sys/types.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>


namespace cdf {

    struct NetworkSession {


        void write(const char* msg, ushort len);

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

    private:
        struct event_base *base;
	    struct evconnlistener *listener;
	    struct event *signal_event;
        struct sockaddr_in sin;

    };

}