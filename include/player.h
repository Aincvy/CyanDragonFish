/**
 * created by aincvy(aincvy@gmail.com) on 2022/9/25
 */

#pragma once

#include <sys/types.h>

#include "network.h"

namespace cdf {

    /**
     * player class 
     */
    struct Player {
        uint playerId = 0;
        NetworkSession* session = nullptr;
        
    };

    struct PlayerManager {

    };

}