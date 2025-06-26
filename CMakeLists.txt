cmake_minimum_required(VERSION 3.31)
project(RaftKVstore)

set(CMAKE_CXX_STANDARD 20)

add_executable(RaftKVstore main.cpp)

find_package(Boost REQUIRED COMPONENTS system filesystem)
target_link_libraries(RaftKVstore PRIVATE Boost::boost Boost::system Boost::filesystem)