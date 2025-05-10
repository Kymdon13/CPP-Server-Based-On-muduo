cmake_minimum_required(VERSION 3.16)
set(BUILD_SHARED_LIBS ON)
# set(CMAKE_C_COMPILER clang)
# set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "CXX standard is ${CMAKE_CXX_STANDARD}")
message(STATUS "C/CXX compiler are ${CMAKE_C_COMPILER} and ${CMAKE_CXX_COMPILER}")

####################
# project setting
####################

project(CppServer
        VERSION 0.1
        DESCRIPTION "A simple server written in C/C++."
        LANGUAGES C CXX
)

####################
# check if you are running `cmake` from a folder that has a CMakeLists.txt
####################
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" PATH_TO_CMAKELISTS_TXT)
if (EXISTS "${PATH_TO_CMAKELISTS_TXT}")
    message(FATAL_ERROR "Please run cmake from a build subdir! \"mkdir build ; cd build ; cmake ..\" \
    Some junk files were created in this folder (CMakeCache.txt, CMakeFiles ...); you should delete those.")
endif ()

# set search path
set(CPPSERVER_BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support")
set(CPPSERVER_CLANG_SEARCH_PATH "/usr/bin")

############################################################
# find dependent tools
############################################################

# ctest
enable_testing() # allow ctest to run tests

# clang-format
if (NOT DEFINED CLANG_FORMAT_BIN)
    # attempt to find the binary if user did not specify
    find_program(CLANG_FORMAT_BIN
                NAMES clang-format
                HINTS ${CPPSERVER_CLANG_SEARCH_PATH})
endif ()
if ("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
    message(WARNING "CppServer/main couldn't find clang-format.")
else ()
    message(STATUS "CppServer/main found clang-format at ${CLANG_FORMAT_BIN}")
endif ()

# # clang-tidy
# FIXME(ban)
# if (NOT DEFINED CLANG_TIDY_BIN)
#     # attempt to find the binary if user did not specify
#     find_program(CLANG_TIDY_BIN
#                 NAMES clang-tidy
#                 HINTS ${CPPSERVER_CLANG_SEARCH_PATH})
# endif ()
# if ("${CLANG_TIDY_BIN}" STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
#     message(WARNING "CppServer/main couldn't find clang-tidy.")
# else ()
#     set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # output compile_commands.json
#     message(STATUS "CppServer/main found clang-fidy at ${CLANG_TIDY_BIN}")
# endif ()

# # cpplint
# FIXME(ban)
# find_program(CPPLINT_BIN
#             NAMES cpplint cpplint.py
#             HINTS ${CPPSERVER_BUILD_SUPPORT_DIR})
# if ("${CPPLINT_BIN}" STREQUAL "CPPLINT_BIN-NOTFOUND")
#     message(WARNING "CppServer/main couldn't find cpplint.")
# else ()
#     message(STATUS "CppServer/main found cpplint at ${CPPLINT_BIN}")
# endif ()

############################################################
# compiler setup
############################################################

### compiler flags
# -fPIC: generate Position Independent Code
# -stdlib=libc++: use libc++ instead of libstdc++
# -Wall: enable all compiler warnings
# -Wextra: enable extra compiler warnings which is more strict than `-Wall`
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -stdlib=libc++ -Wall -Wextra -pthread")
# -O0: disable compiler optimizations
# -ggdb: generate debugging info for the GDB debugger
# -fsanitize=address: enable AddressSanitizer, a tool for detecting memory errors
# -fno-optimize-sibling-calls: disables sibling call optimization, also helpfull for debugging
# BUG(wzy) disable the ASan, cuz it is not compatible with libc++ and gtest, any throw now will cause ASan error
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls")
### linker flags
# -Wl,-rpath,/usr/lib/llvm-17/lib: set the runtime library search path to /usr/lib/llvm-17/lib
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++ -lc++abi -Wl,-rpath,/usr/lib/llvm-17/lib")
##### Or you can write like this
# add_compile_options(-stdlib=libc++ ...)
# add_link_options(-stdlib=libc++ -lc++ -lc++abi ...)

# enable ASan support for libc++
add_compile_definitions(_LIBCPP_HAS_ASAN)

# generate Position Independent Code is essential for shared libs
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")

# set output dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set include dirs
include_directories(${PROJECT_SOURCE_DIR}/src)
# set compiler flags
include_directories(SYSTEM /usr/lib/llvm-17/include/c++/v1)
link_directories(/usr/lib/llvm-17/lib)

# add sub CMakeLists.txt
add_subdirectory(src)
add_subdirectory(test)

############################################################
# make tools
############################################################

####################
# `make format`
# `make check-format`
####################
string(CONCAT CPPSERVER_FORMAT_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src,"
    "${CMAKE_CURRENT_SOURCE_DIR}/test"
    )

# run clang format and updates files in place, custom target usually won't add to make all target, but ALL option will do so
add_custom_target(format ALL
                ${CPPSERVER_BUILD_SUPPORT_DIR}/run_clang_format.py
                ${CLANG_FORMAT_BIN}
                ${CPPSERVER_BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
                --source_dirs
                ${CPPSERVER_FORMAT_DIRS}
                --fix
                --quiet
                )

# run clang format and exits with a non-zero exit code if any files need to be reformatted
# FIXME(ban)
# add_custom_target(check-format
#                 ${CPPSERVER_BUILD_SUPPORT_DIR}/run_clang_format.py
#                 ${CLANG_FORMAT_BIN}
#                 ${CPPSERVER_BUILD_SUPPORT_DIR}/clang_format_exclusions.txt
#                 --source_dirs
#                 ${CPPSERVER_FORMAT_DIRS}
#                 --quiet
#                 )

####################
# `make clang-tidy`
####################
# runs clang-tidy and exits with a non-zero exit code if any errors are found
# note that clang-tidy automatically looks for a .clang-tidy file in parent directories
# FIXME(ban)
# add_custom_target(clang-tidy
#                 ${CPPSERVER_BUILD_SUPPORT_DIR}/run_clang_tidy.py # run LLVM's clang-tidy script
#                 -clang-tidy-binary ${CLANG_TIDY_BIN} # using our clang-tidy binary
#                 -p ${CMAKE_BINARY_DIR} # using cmake's generated compile commands
#                 )

####################
# `make cpplint`
####################
# FIXME(ban)
# file(GLOB_RECURSE CPPSERVER_LINT_FILES
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cc"
#     "${CMAKE_CURRENT_SOURCE_DIR}/test/*.h"
#     "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cc"
#     )

# balancing act: cpplint.py takes a non-trivial time to launch
# so process 12 files per invocation, while still ensuring parallelism
# FIXME(ban)
# add_custom_target(cpplint
#                 echo '${CPPSERVER_LINT_FILES}' | xargs -n12 -P8
#                 ${CPPLINT_BIN}
#                 --verbose=2 --quiet
#                 --linelength=120
#                 --filter=-legal/copyright,-build/include_subdir,-readability/casting
#                 )

############################################################
# my own scripts
############################################################

# ### main executable

# file(GLOB SOURCES_SERVER "src/*.cc")
# add_executable(server ${SOURCES_SERVER} "src/main-server.cpp" )

# file(GLOB SOURCES_CLIENT "src/*.cc")
# add_executable(client ${SOURCES_CLIENT} "src/main-client.cpp")

# ### test

# find_package(GTest REQUIRED) # search /usr/lib or other lib path to find gtest lib file
# include(CTest)
# include(GoogleTest) # load internal GoogleTest.cmake module, provide cmake function like gtest_discover_tests()

# # test 100000 concurrent connection
# add_executable(
#     test100000
#     ${SOURCES_SERVER}
#     "test/test100000.cc"
# )

# # test unit for ThreadPool
# add_executable(
#     test_ThreadPool
#     "src/ThreadPool.cc"
#     "test/ThreadPool.cc"
# )

# # link lib file to executable
# # target_link_libraries(
# #     test_ThreadPool
# #     gtest # core lib
# #     gtest_main # provide predefined main() entry so that you do not have to write a main() yourself
# #     pthread
# # )

# # auto discover test unit in executable and register it in ctest
# # gtest_discover_tests(test_ThreadPool)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
