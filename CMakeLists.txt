cmake_minimum_required(VERSION 3.31)
project(RaftKVstore)

set(CMAKE_CXX_STANDARD 20)

add_executable(RaftKVstore main.cpp
        kvstore/KVStore.cpp
        kvstore/KVStore.h
        raft/RaftNode.cpp
        raft/RaftNode.h
        raft/RaftServer.cpp
        raft/RaftServer.h
        rpc/Message.h

)

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(nlohmann_json REQUIRED)

target_link_libraries(RaftKVstore PRIVATE
        Boost::boost
        Boost::system
        Boost::filesystem
        nlohmann_json::nlohmann_json)

add_executable(raft_client
        client/RaftClient.cpp
        ${COMMON_SOURCES}
)
target_link_libraries(raft_client
        PRIVATE Boost::filesystem Boost::system
)

add_executable(test
        test/test.cpp
        ${COMMON_SOURCES}
)

target_link_libraries(
        test
        PRIVATE Boost::filesystem Boost::system
)