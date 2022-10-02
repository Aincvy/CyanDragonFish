cmake_minimum_required(VERSION 3.10)
SET(CMAKE_MODULE_PATH APPEND "${PROJECT_SOURCE_DIR}/cmake/" )

SET(CyanDragonFishRelease false)

project(cyanDragonFish)

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

file(GLOB msgSources "src/msg/*.cc")
MESSAGE(STATUS "protobuf msg: ${msgSources}")

target_sources(cyanDragonFish PRIVATE 
    src/player.cpp
    src/network.cpp
    src/config.cpp
    src/log.cpp
    src/server.cpp

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

target_include_directories(
    cyanDragonFish PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/msg
    ${YAML_CPP_INCLUDE_DIR}
    ${Protobuf_INCLUDE_DIRS}
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
    protobuf::libprotobuf
)

target_link_libraries(cyanDragonFish PRIVATE ${EXTRA_LIBS})
MESSAGE(STATUS "Found extra-lib at: ${EXTRA_LIBS}")