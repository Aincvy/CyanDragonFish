cmake_minimum_required(VERSION 3.10)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/" )
MESSAGE(STATUS "MODULE PATH: ${CMAKE_MODULE_PATH}")
SET(CMAKE_PREFIX_PATH "/opt/mongo/lib/cmake/")
MESSAGE(STATUS "PREFIX PATH: ${CMAKE_PREFIX_PATH}")

SET(CyanDragonFishRelease false)

project(cyanDragonFish
    VERSION 0.0.1
    DESCRIPTION "A WIP Game."
    HOMEPAGE_URL https://github.com/Aincvy/CyanDragonFish.git
    LANGUAGES CXX
)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILECOMMANDS ON)

add_executable(cyanDragonFish program.cpp)

if(${CyanDragonFishRelease}) 
    MESSAGE(STATUS "CyanDragonFishRelease")
    set(CMAKE_BUILD_TYPE "Release")
    target_compile_definitions(cyanDragonFish PUBLIC CyanDragonFishRelease=1)
else ()
    MESSAGE(STATUS "CyanDragonFishDebug")
    set(CMAKE_BUILD_TYPE "Debug")
    target_compile_definitions(cyanDragonFish PUBLIC CyanDragonFishDebug=1)
endif()
target_compile_definitions(cyanDragonFish PUBLIC V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)

file(GLOB msgSources "src/msg/*.cc")
MESSAGE(STATUS "protobuf msg: ${msgSources}")

target_sources(cyanDragonFish PRIVATE 
    src/player.cpp
    src/network.cpp
    src/config.cpp
    src/log.cpp
    src/server.cpp
    src/executor.cpp
    src/js_helper.cpp
    src/util.cpp
    src/domain.cpp

    ${msgSources}
)

find_package(Libevent 2.1.12 REQUIRED COMPONENTS core)

# yaml-cpp
find_package(yaml-cpp REQUIRED)
MESSAGE(STATUS "Found yaml-cpp at: ${YAML_CPP_INCLUDE_DIR}")

find_package(spdlog REQUIRED)

# abseil
find_package(absl REQUIRED)

find_package(protobuf REQUIRED)

# V8
SET(V8_DIR "/opt/v8")
find_package(V8 REQUIRED)

find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)


add_subdirectory(dependencies/v8pp)

target_include_directories(
    cyanDragonFish PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/msg
    ${YAML_CPP_INCLUDE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${V8_INCLUDE_DIR}
)

SET(EXTRA_LIBS ${YAML_CPP_LIBRARIES} 
    libevent::core 
    libevent::pthreads 
    spdlog::spdlog
    absl::base
    absl::strings
    absl::hash
    absl::flat_hash_map
    absl::flat_hash_set
    absl::btree       
    absl::random_random
    absl::time

    protobuf::libprotobuf
    ${V8_LIBRARIES}
    mongo::bsoncxx_shared
    mongo::mongocxx_shared
    v8pp
)

target_link_libraries(cyanDragonFish PRIVATE ${EXTRA_LIBS})
MESSAGE(STATUS "Found extra-lib at: ${EXTRA_LIBS}")