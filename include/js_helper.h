/**
 * created by aincvy(aincvy@gmail.com) on 2022/10/3
 */

#pragma once

#include <string>
#include <string_view>
#include <v8.h>

#include <v8pp/call_v8.hpp>

// #include "player.h"
#include "v8pp/convert.hpp"

namespace cdf {
    using V8Function = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;

    struct DbClientWrapper;
    struct PlayerThreadLocal;

    struct DbCollectionWrapper {

        std::string collectionName;
        // DbClientWrapper* client = nullptr;
        PlayerThreadLocal* threadLocal = nullptr;
        // v8::Local<v8::Function> entityCtor;
        V8Function entityCtor;

        DbCollectionWrapper(std::string_view cName, PlayerThreadLocal* threadLocal);

        v8::Local<v8::Value> find(v8::Local<v8::Value> condition);
        v8::Local<v8::Value> insert(v8::Local<v8::Value> value);

        v8::Local<v8::Value> getEntityCtor();
        void setEntityCtor(v8::Local<v8::Function> entityCtor);
    };

    void initJsEngine(std::string_view execPath);

    void destroyJsEngine();

    /**
     * Domain classes.
     */
    void registerDomain(v8::Isolate* isolate);

    /**
     * Database operations.
     */
    void registerDatabaseOpr(v8::Isolate* isolate);

    void registerDatabaseObject(v8::Isolate* isolate, PlayerThreadLocal* threadLocal);

    void registerUtilFunctions(v8::Isolate* isolate, PlayerThreadLocal* threadLocal);

    /**
     * load and execute all js files in `javascripts` folder.
     */
    void loadJsFiles(v8::Isolate* isolate, std::string_view folder);

    void loadJsFile(v8::Isolate* isolate, std::string_view path);

    void loadJsFileInGlobal(v8::Isolate* isolate, std::string_view path);

    void reportException(v8::Isolate* isolate, std::string_view path, v8::TryCatch const& tryCatch);

}