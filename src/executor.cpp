#include "log.h"
#include "executor.h"

#include "command.pb.h"
#include "gameMsg.pb.h"
#include "player.h"

#include "server.h"

namespace cdf {
    using namespace msg;

    void onReqHello(Player* player, ReqHello const& req) {
        ResHello resHello;
        resHello.set_is_full(false);
        resHello.set_server_version(SERVER_VERSION_STR);

        player->write(Command::Hello, 0, &resHello);
    }

    void onReqLoginAccount(Player* player, ReqLoginAccount const& req) {
        
    }

    void initExecutor() {
        bindCommand(msg::Command::Hello, onReqHello );
        bindCommand(msg::Command::LoginAccount, onReqLoginAccount );

    }

}