cmake_minimum_required(VERSION 3.16)
project(CppServer)
set(CMAKE_CXX_STANDARD 11)

include_directories(src/)

file(GLOB_RECURSE SOURCES_SERVER "src/*.cc")
add_executable(server ${SOURCES_SERVER} "src/main-server.cpp" )

file(GLOB_RECURSE SOURCES_CLIENT "src/*.cc")
add_executable(client ${SOURCES_CLIENT} "src/main-client.cpp")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
