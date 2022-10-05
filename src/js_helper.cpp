#include "log.h"
#include "js_helper.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <v8.h>
#include <v8-platform.h>
#include <libplatform/libplatform.h>

#include "domain.h"
#include "v8pp/convert.hpp"

#include <v8pp/class.hpp>
#include <v8pp/module.hpp>

namespace cdf {

    void initJsEngine(std::string_view execPath) {
        SPDLOG_INFO("Init Js Engine, execPath: {}", execPath);
        auto path = execPath.data();
        // Initialize V8.
        v8::V8::InitializeICUDefaultLocation(path);
        v8::V8::InitializeExternalStartupData(path);
        std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(platform.get());
        v8::V8::Initialize();

        SPDLOG_INFO("Init Js Engine Over");
    }

    void destroyJsEngine() {
        SPDLOG_INFO("Destroy Js Engine");
        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();
        SPDLOG_INFO("Destroy Js Engine Over");
    }

    void registerDomain(v8::Isolate* isolate) {
        SPDLOG_DEBUG("register domain to {:p}", fmt::ptr(isolate));

        v8pp::class_<Account> accountClass(isolate);
        accountClass.ctor<>()
            .var("id", &Account::id)    
            .var("username", &Account::username)    
            .var("password", &Account::password)    
            .var("salt", &Account::salt)    
            .var("createTime", &Account::createTime)    
            .var("lastLoginTime", &Account::lastLoginTime)    
        ;

        v8pp::class_<Role> roleClass(isolate);
        roleClass.ctor<>()
            .var("playerId", &Role::playerId)
            .var("nickname", &Role::nickname)
            .var("level", &Role::level)
            .var("exp", &Role::exp)
            ;

        v8pp::class_<Item> itemClass(isolate);
        itemClass.ctor<>()
            .var("playerId", &Item::playerId)
            .var("itemId", &Item::itemId)
            .var("count", &Item::count)
        ;

        v8pp::module myLib(isolate);
        myLib.class_("Account", accountClass);
        myLib.class_("Role", roleClass);
        myLib.class_("Item", itemClass);
        myLib.function("nextPlayerId", nextPlayerId);
        
        auto context = isolate->GetCurrentContext();
        
        context->Global()->Set(context, v8pp::to_v8(isolate, "domain"), myLib.new_instance()).ToChecked();
    }

    void registerDatabaseOpr(v8::Isolate* isolate, PlayerThreadLocal* threadLocal) {
        SPDLOG_DEBUG("register database operation to {:p}", fmt::ptr(isolate));


    }

    void registerUtilFunctions(v8::Isolate* isolate) {
        SPDLOG_DEBUG("register util functions to {:p}", fmt::ptr(isolate));

    }

    void loadJsFiles(v8::Isolate* isolate) {
        SPDLOG_DEBUG("load all js files to {:p}", fmt::ptr(isolate));


    }

}