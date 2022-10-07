#include "log.h"
#include "js_helper.h"

#include <bsoncxx/document/view.hpp>
#include <fmt/format.h>
#include <iterator>
#include <mongocxx/cursor.hpp>
#include <spdlog/spdlog.h>

#include <sstream>
#include <string>
#include <filesystem>

#include <string_view>
#include <v8.h>
#include <v8-platform.h>
#include <libplatform/libplatform.h>

#include "domain.h"

#include "util.h"
#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>
#include <v8pp/json.hpp>
#include <v8pp/call_v8.hpp>

#include <bsoncxx/json.hpp>
#include <vector>

#include <absl/container/flat_hash_set.h>
#include <absl/strings/match.h>

namespace cdf {

    using namespace v8;
    static std::unique_ptr<v8::Platform> platform;

    namespace {
        
        std::string currentStackTrace(v8::Isolate* isolate) {
            auto stackTrace = v8::StackTrace::CurrentStackTrace(isolate, 10, v8::StackTrace::kOverview);

            std::string fileName;
            int lineNumber = 0;
            std::string functionName;
            if (stackTrace->GetFrameCount() > 0) {
                auto frame = stackTrace->GetFrame(isolate, 0);
                if (!frame->GetScriptName().IsEmpty()) {
                    fileName = v8pp::from_v8<std::string>(isolate, frame->GetScriptName());
                }
                lineNumber = frame->GetLineNumber();
                if (!frame->GetFunctionName().IsEmpty()) {
                    functionName = v8pp::from_v8<std::string>(isolate, frame->GetFunctionName());
                }
            }
            std::stringstream ss;
            ss << "[" << fileName << ":" <<  lineNumber << "(" << functionName << ")]";
            return ss.str();
        } 

        // print func
        void v8Print(const FunctionCallbackInfo<Value>& args) {
            auto isolate = args.GetIsolate();            
            auto context = isolate->GetCurrentContext();

            auto trace = currentStackTrace(isolate);
            for( int i = 0; i < args.Length(); i++){
                std::stringstream ss;
                ss << trace << " ";

                auto arg = args[i];

                if (arg->IsString() || arg->IsStringObject()) {
                    std::string msg = v8pp::from_v8<std::string>(isolate, arg);
                    ss << msg;
                } else if (arg->IsNumber() || arg->IsNumberObject()) {
                    if (arg->IsInt32() ) {
                        int msg = arg->Int32Value(context).ToChecked();
                        ss << msg;
                    } else if (arg->IsUint32()) {
                        uint msg = arg->Uint32Value(context).ToChecked();
                        ss << msg;
                    } else {
                        auto msg = arg->NumberValue(context).ToChecked();
                        ss << msg;
                    }
                } else if (arg->IsObject()) {
                    auto str = v8pp::json_str(isolate, arg);
                    auto msg = str.c_str();
                    ss << msg;
                } else if (arg->IsUndefined()) {
                    constexpr const char* msg = "undefined";
                    ss << msg;
                } else if (arg->IsNull()) {
                    constexpr const char* msg = "null";
                    ss << msg;
                } else if (arg->IsExternal()) {
                    constexpr const char* msg = "external";
                    ss << msg;
                } else if (arg->IsBoolean() || arg->IsBooleanObject()) {
                    const char* msg = arg->BooleanValue(isolate) ? "true" : "false";
                    ss << msg;
                }
                else {
                    SPDLOG_LOGGER_ERROR(logError, "js print function, msg type not handled.");
                }

                logScript->info(ss.str());
            }
        }

        /**
         *
         * @from https://github.com/danbev/learning-v8/blob/master/run-script.cc
         * @param isolate
         * @param name
         * @return
         */
        v8::MaybeLocal<v8::String> readFile(v8::Isolate *isolate, const char *name) {
            FILE *file = fopen(name, "rb");
            if (file == NULL) {
                return {};
            }

            fseek(file, 0, SEEK_END);
            size_t size = ftell(file);
            rewind(file);

            char *chars = new char[size + 1];
            chars[size] = '\0';
            for (size_t i = 0; i < size;) {
                i += fread(&chars[i], 1, size - i, file);
                if (ferror(file)) {
                    fclose(file);
                    return {};
                }
            }
            fclose(file);
            v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(isolate,
                                                                        chars,
                                                                        v8::NewStringType::kNormal,
                                                                        static_cast<int>(size));
            delete[] chars;
            return result;
        }

    }

    void initJsEngine(std::string_view execPath) {
        SPDLOG_INFO("Init Js Engine, execPath: {}", execPath);
        auto path = execPath.data();
        // Initialize V8.
        v8::V8::InitializeICUDefaultLocation(path);
        v8::V8::InitializeExternalStartupData(path);
        platform = v8::platform::NewDefaultPlatform();
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
    
        v8pp::class_<Entity> entityClass(isolate);
        entityClass.var("_id", &Entity::_id)
            .var("createTime", &Entity::createTime)
            .var("updateTime", &Entity::updateTime)
        ;

        v8pp::class_<Account> accountClass(isolate);
        accountClass.ctor<>()
            .inherit<Entity>()
            .var("id", &Account::id)    
            .var("username", &Account::username)    
            .var("password", &Account::password)    
            .var("salt", &Account::salt)    
            .var("lastLoginTime", &Account::lastLoginTime)    
        ;

        v8pp::class_<Role> roleClass(isolate);
        roleClass.ctor<>()
            .inherit<Entity>()
            .var("accountId", &Role::accountId)
            .var("playerId", &Role::playerId)
            .var("nickname", &Role::nickname)
            .var("level", &Role::level)
            .var("exp", &Role::exp)
            ;

        v8pp::class_<Item> itemClass(isolate);
        itemClass.ctor<>()
            .inherit<Entity>()
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

    void registerDatabaseOpr(v8::Isolate* isolate) {
        SPDLOG_DEBUG("register database operation to {:p}", fmt::ptr(isolate));

        v8pp::class_<DbCollectionWrapper> collectionWrapperClass(isolate);
        collectionWrapperClass
            .function("_find", & DbCollectionWrapper::find)
            .function("_insert", & DbCollectionWrapper::insert)
            .property("entityCtor", &DbCollectionWrapper::getEntityCtor, &DbCollectionWrapper::setEntityCtor)
            ;
        // collectionWrapperClass.auto_wrap_objects(true);
        
        v8pp::module databaseMod(isolate);
        databaseMod.class_("collection", collectionWrapperClass);

        auto context = isolate->GetCurrentContext();
        context->Global()->Set(context, v8pp::to_v8(isolate, "database"), databaseMod.new_instance()).ToChecked();
    }

    void registerDatabaseObject(v8::Isolate* isolate, PlayerThreadLocal* threadLocal) {
        SPDLOG_DEBUG("register database object to {:p}", fmt::ptr(isolate));
        auto context = isolate->GetCurrentContext();
        v8::Local<v8::Object> db = v8::Object::New(isolate);
        db->Set(context, v8pp::to_v8(isolate, "account"), 
            v8pp::class_<DbCollectionWrapper>::import_external(isolate, new DbCollectionWrapper("account", threadLocal))).ToChecked();
        
        context->Global()->Set(context, v8pp::to_v8(isolate, "db"), db).ToChecked();
    }

    void registerUtilFunctions(v8::Isolate* isolate) {
        SPDLOG_DEBUG("register util functions to {:p}", fmt::ptr(isolate));

        auto context = isolate->GetCurrentContext();
        auto global = context->Global();
        
        auto printFunc = v8pp::wrap_function(isolate, "print", &v8Print);
        global->Set(context, v8pp::to_v8(isolate, "print"), printFunc).ToChecked();

        auto currentTimeFunc = v8pp::wrap_function(isolate, "currentTime", currentTime);
        global->Set(context, v8pp::to_v8(isolate, "currentTime"), currentTimeFunc).ToChecked();

    }

    void loadJsFiles(v8::Isolate* isolate, std::string_view folder) {
        SPDLOG_DEBUG("load all js files to {:p}", fmt::ptr(isolate));

        absl::flat_hash_set<std::string> ignoreFiles;
        ignoreFiles.insert("prelib.js");
        ignoreFiles.insert("postlib.js");
        

        namespace fs = std::filesystem;
        for(const auto& item: fs::directory_iterator(folder)) {
            if (!item.is_regular_file())
            {
                continue;
            }
            std::string filename = item.path().filename().generic_string();
            if (ignoreFiles.contains(filename))
            {
                continue;
            }
            
            if (!absl::EndsWith(filename, ".js"))
            {
                continue;
            }

            loadJsFile(isolate, item.path().generic_string());
        }
    }

    void reportException(v8::Isolate* isolate, std::string_view path, v8::TryCatch const& tryCatch) {
        auto exception = tryCatch.Exception();
        std::string s;
        auto context = isolate->GetCurrentContext();
        if (exception->IsString() || exception->IsStringObject()) {
            s = v8pp::from_v8<std::string>(isolate, tryCatch.Exception());
        } else if (exception->IsObject())
        {
            s = v8pp::from_v8<std::string>(isolate, JSON::Stringify(context, exception).ToLocalChecked());
            SPDLOG_ERROR("exception not handle[object]");
        } else {
            SPDLOG_ERROR("exception not handle");
        }
        
        SPDLOG_LOGGER_ERROR(logError, "path: {}, message: {}",path, s);    
        
    }

    void loadJsFile(v8::Isolate* isolate, std::string_view path) {
        logScript->debug("load js file: {}", path);
        auto fileSource = readFile(isolate, path.data());
        if (fileSource.IsEmpty()) {
            SPDLOG_LOGGER_ERROR(logError, "could not load js file: {}",  path);
            return;
        }

        v8::HandleScope handleScope(isolate);
        TryCatch tryCatch(isolate);

        auto context = isolate->GetCurrentContext();
        auto scriptPath = v8::String::NewFromUtf8(isolate, path.data()).ToLocalChecked();
        ScriptOrigin origin{ scriptPath, 0, 0 };
        v8::ScriptCompiler::Source source(fileSource.ToLocalChecked(), origin);
        v8::Local<v8::String> paramModule = v8::String::NewFromUtf8(isolate, "module").ToLocalChecked();
        v8::Local<v8::String> paramExports = v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked();
        v8::Local<v8::String> arguments[] { paramModule, paramExports};
        auto mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, std::size(arguments), arguments,
                                                                       0, nullptr);
        if (mayFunction.IsEmpty()) {
            SPDLOG_LOGGER_ERROR(logError, "could not compile js file: {}",  path);
            if (tryCatch.HasCaught()) {
                reportException(isolate, path, tryCatch);
            }
            return;
        }

        auto func = mayFunction.ToLocalChecked();
        auto recv = Object::New(isolate);
        auto exports = Object::New(isolate);
        auto module = Object::New(isolate);
        module->Set(context, v8pp::to_v8(isolate, "exports"), exports).ToChecked();
        
        v8pp::call_v8(isolate, func, recv, module, exports);

        if (tryCatch.HasCaught()) {
            reportException(isolate, path,  tryCatch);
        } else {
            // register exports to global
            auto g = context->Global();
            auto names = exports->GetOwnPropertyNames(context).ToLocalChecked();
            SPDLOG_DEBUG("names length:  {}", names->Length());
            for (uint i = 0; i < names->Length(); i++)
            {
                auto name = names->Get(context, i).ToLocalChecked();
                auto v = exports->Get(context, name).ToLocalChecked();
                if (name->IsString() || name->IsStringObject()) {
                    auto nameString = v8pp::from_v8<std::string>(isolate, name);
                    SPDLOG_DEBUG(nameString);
                }
                g->Set(context, name, v).ToChecked();
            }
        }
    }

    DbCollectionWrapper::DbCollectionWrapper(std::string_view cName, PlayerThreadLocal* threadLocal)
        : collectionName(cName), threadLocal(threadLocal)
    {
        
    }

    v8::Local<v8::Value> DbCollectionWrapper::find(v8::Local<v8::Value> condition) {
        auto& database = threadLocal->getDatabase();
        auto isolate = threadLocal->getIsolate();
        auto context = isolate->GetCurrentContext();
        std::vector<bsoncxx::document::view> result;

        auto setResult = [&result](mongocxx::cursor & cursor) {
            for (auto item : cursor) {
                result.push_back(item);
            }
        };

        auto convert = [isolate, &context](bsoncxx::document::view const& view) {
            auto jsonString = bsoncxx::to_json(view);
            auto tmp = v8::JSON::Parse(context, v8::String::NewFromUtf8(isolate, jsonString.c_str()).ToLocalChecked());
            return tmp.ToLocalChecked();
        };

        if (condition.IsEmpty() || condition->IsNullOrUndefined()) {
            auto c = database[collectionName].find({});    
            setResult(c);
        } else {
            auto maybeResult = v8::JSON::Stringify(context, condition);
            if (maybeResult.IsEmpty()) {
                SPDLOG_LOGGER_ERROR(logError, "parse object to json failed.");
                return {};
            }

            std::string json = *(v8::String::Utf8Value(isolate, maybeResult.ToLocalChecked()));
            auto view = bsoncxx::from_json(json);
            auto c = database[collectionName].find(view.view());
            setResult(c);
        }
        
        if (result.empty()) {
            return v8::Undefined(isolate);
        } else if (result.size() == 1)
        {
            // 1 result
            return convert(result.back());
        } else {
            // 
            auto array = v8::Array::New(isolate, result.size());
            auto index = 0;
            for (auto & item : result) {
                array->Set(context, index++, convert(item)).ToChecked();
            }
            return array;
        }

        // return {};
    }

    v8::Local<v8::Value> DbCollectionWrapper::insert(v8::Local<v8::Value> value) {
        auto& database = threadLocal->getDatabase();
        auto isolate = threadLocal->getIsolate();
        auto context = isolate->GetCurrentContext();

        auto mayJsonStr = JSON::Stringify(context, value);
        if (mayJsonStr.IsEmpty()) {
            logScript->error("{} insert failed, object cannot be to json", currentStackTrace(isolate));
            return Undefined(isolate);
        }

        std::string json = *(v8::String::Utf8Value(isolate, mayJsonStr.ToLocalChecked()));
        auto doc = bsoncxx::from_json(json);
        
        auto result = database[collectionName].insert_one(doc.view());
        if (result.has_value()) {
            std::string s = result->inserted_id().get_oid().value.to_string();
            return v8pp::to_v8(isolate, s);
        } else {
            logScript->error("{} insert failed, mongodb say insert no reponse", currentStackTrace(isolate));
            return Undefined(isolate);
        }
    }

    v8::Local<v8::Value> DbCollectionWrapper::getEntityCtor() {
        auto isolate = Isolate::GetCurrent();
        Local<Value> f = Undefined(isolate);
        if (entityCtor.IsEmpty())
        {
        } else {
            f = entityCtor.Get(isolate);
        }
        
        return f;
    }

    void DbCollectionWrapper::setEntityCtor(v8::Local<v8::Function> entityCtor) {
        auto isolate = Isolate::GetCurrent();
        this->entityCtor = Function(isolate, entityCtor);
        // SPDLOG_DEBUG("111: {}", this->entityCtor->IsFunction());
    }

}