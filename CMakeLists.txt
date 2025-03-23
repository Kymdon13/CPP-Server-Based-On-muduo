cmake_minimum_required(VERSION 3.16)
project(CppServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

### main executable

include_directories(src/)

file(GLOB SOURCES_SERVER "src/*.cc")
add_executable(server ${SOURCES_SERVER} "src/main-server.cpp" )

file(GLOB SOURCES_CLIENT "src/*.cc")
add_executable(client ${SOURCES_CLIENT} "src/main-client.cpp")

### test

find_package(GTest REQUIRED) # search /usr/lib or other lib path to find gtest lib file
include(CTest)
include(GoogleTest) # load internal GoogleTest.cmake module, provide cmake function like gtest_discover_tests()

enable_testing() # allow ctest to run tests

# test 100000 concurrent connection
add_executable(
    test100000
    ${SOURCES_SERVER}
    "test/test100000.cc"
)

# test unit for ThreadPool
add_executable(
    test_ThreadPool
    "src/ThreadPool.cc"
    "test/ThreadPool.cc"
)

# link lib file to executable
# target_link_libraries(
#     test_ThreadPool
#     gtest # core lib
#     gtest_main # provide predefined main() entry so that you do not have to write a main() yourself
#     pthread
# )

# auto discover test unit in executable and register it in ctest
# gtest_discover_tests(test_ThreadPool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
