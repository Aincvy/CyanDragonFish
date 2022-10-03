/**
 * created by aincvy(aincvy@gmail.com) on 2022/10/3
 */

#pragma once
#include "log.h"
#include <spdlog/spdlog.h>

#include <functional>
#include <type_traits>

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

#include "player.h"

#include "gameMsg.pb.h"



namespace cdf {

    template<class T, typename std::enable_if<std::is_base_of_v<google::protobuf::Message, T>, bool>::type = true>
    void bindCommand(int command, void(*handler)(cdf::Player* player, T const& msg) ) {
        using namespace msg;
        onCommand(command, [command, handler](Player* player, GameMsgReq const& msg) {   
            T t;
            if(t.ParseFromString(msg.content())) {
#ifdef CyanDragonFishDebug
                google::protobuf::TextFormat::Printer printer;
                printer.SetSingleLineMode(true);
                if(std::string tmp; printer.PrintToString(t, &tmp)){
                    logMessage->debug("Recv Fm {},[CV] {} [{}] {}", player->getPlayerId(), msg.command(),t.GetTypeName(), tmp);
                    // SPDLOG_LOGGER_DEBUG(logMessage, "Recv Fm {},[CV] {} [{}] {}", player->getPlayerId(), msg.command(),t.GetTypeName(), tmp);
                } else {
                    logMessage->debug("Recv Fm {},[CV] {} [{}] {}", player->getPlayerId(), msg.command(),t.GetTypeName(), t.ShortDebugString());
                    // SPDLOG_LOGGER_DEBUG(logMessage, "Recv Fm {},[CV] {} [{}] {}", player->getPlayerId(), msg.command(),t.GetTypeName(), t.ShortDebugString());
                }
#endif // CyanDragonFishDebug   

                handler(player, t);
            } else {
                SPDLOG_LOGGER_ERROR(logMessage, "parse message failed. bind command: {}, gameMsg command: {}", command, msg.command());
            }
        });
    }


    void initExecutor();

}