#include "log.h"
#include "domain.h"
#include "server.h"

#include <atomic>

#include <mongocxx/client.hpp>

namespace cdf {

    static constexpr const int ID_BASE = 1000000;
    static std::atomic_uint playerIdIncr = 0;

    void initPlayerId(mongocxx::pool::entry entry) {
        auto database = entry->database(currentServer()->launchConfig.database.db);
        playerIdIncr = database["role"].estimated_document_count() + ID_BASE;
    }

    uint nextPlayerId() {
        return ++playerIdIncr;
    }

}