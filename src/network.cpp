#include "network.h"

#include "common.h"
#include "server.h"
#include "spdlog/spdlog.h"
#include "log.h"
#include <cassert>
#include <cstdlib>
#include <arpa/inet.h>

#include <event2/event.h>
#include <mutex>
#include <signal.h>

namespace cdf {
    
    static void read_cb(struct bufferevent *bev, void *arg)
    {
        struct evbuffer *in = bufferevent_get_input(bev);
        // evbuffer_write(in, fileno(ctx->fout));
        
    }

    static void conn_writecb(struct bufferevent *bev, void *user_data)
    {
        struct evbuffer *output = bufferevent_get_output(bev);
        if (evbuffer_get_length(output) == 0) {
            printf("flushed answer\n");
            bufferevent_free(bev);
        }
    }
    
    static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
    {
        if (events & BEV_EVENT_EOF) {
            printf("Connection closed.\n");
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

        if(currentServer()->networkManager.addNewSession(bev, fd)){
            bufferevent_setcb(bev, read_cb, conn_writecb, conn_eventcb, NULL);
            bufferevent_enable(bev, EV_WRITE | EV_READ);
        } else {
            logError->error("add new session failed");
        }
    }

    static void signal_cb(evutil_socket_t, short, void *user_data){
        struct event_base *base = (struct event_base *)user_data;
        struct timeval delay = { 1, 0 };

        printf("Caught an interrupt signal; exiting cleanly in 1 seconds.\n");

        event_base_loopexit(base, &delay);
    }

    void NetworkManager::serv() {
        assert(base);
        logConsole->info("network serv");
        event_base_dispatch(base);
    }

    void NetworkManager::init() {
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

    bool NetworkManager::addNewSession(struct bufferevent *bev, int fd) {
        if(!bev) {
            return false;
        }

        // std::lock_guard<std::mutex> guard(this->sessionMapMutex);
        sessionMap[bev] = { bev, fd};
        return true;
    }

    NetworkSession::NetworkSession()
    {
        
    }

    NetworkSession::NetworkSession(struct bufferevent *bev, int fd)
        : bev(bev), fd(fd)
    {
        
    }

    NetworkSession::~NetworkSession()
    {
        
    }

    void NetworkSession::close() {
        reset();
    }

    void NetworkSession::reset() {
        // todo 
    }

}