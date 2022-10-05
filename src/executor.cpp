#include "log.h"
#include "executor.h"

#include "command.pb.h"
#include "gameMsg.pb.h"
#include "player.h"

#include "server.h"

#include <mongocxx/collection.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <spdlog/spdlog.h>

namespace cdf {
    using namespace msg;

    using bsoncxx::builder::stream::close_array;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_array;
    using bsoncxx::builder::stream::open_document;

    void onReqHello(Player* player, ReqHello const& req) {
        ResHello resHello;
        resHello.set_is_full(false);
        resHello.set_server_version(SERVER_VERSION_STR);

        auto& db = player->getPlayerThread()->getDatabase();
        auto result = db["statistics"].update_one(document{} << finalize, 
                document{} << "$inc" << open_document << "hello" << 1 << close_document << finalize);
        if (result) {
            SPDLOG_DEBUG("modified_count: {}", result->modified_count());
            if (result->modified_count() == 0) {
                // create one
                db["statistics"].insert_one(document{} << "hello" << 1 << finalize);
            }
        }
        
        player->write(Command::Hello, 0, &resHello);
    }

    void onReqLoginAccount(Player* player, ReqLoginAccount const& req) {
        
    }

    void initExecutor() {
        bindCommand(msg::Command::Hello, onReqHello );
        bindCommand(msg::Command::LoginAccount, onReqLoginAccount );

    }

}