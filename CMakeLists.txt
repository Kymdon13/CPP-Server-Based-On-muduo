cmake_minimum_required(VERSION 3.16)
set(BUILD_SHARED_LIBS ON)
# set(CMAKE_C_COMPILER clang)
# set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
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
if(EXISTS "${PATH_TO_CMAKELISTS_TXT}")
    message(FATAL_ERROR "Please run cmake from a build subdir! \"mkdir build ; cd build ; cmake ..\" \
    Some junk files were created in this folder (CMakeCache.txt, CMakeFiles ...); you should delete those.")
endif()

# set search path
set(CPPSERVER_BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build_support")
set(CPPSERVER_CLANG_SEARCH_PATH "/usr/bin")

############################################################
# find dependent tools
############################################################

# ctest
enable_testing() # allow ctest to run tests

# clang-format
if(NOT DEFINED CLANG_FORMAT_BIN)
    # attempt to find the binary if user did not specify
    find_program(CLANG_FORMAT_BIN
        NAMES clang-format
        HINTS ${CPPSERVER_CLANG_SEARCH_PATH})
endif()
if("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
    message(WARNING "CppServer/main couldn't find clang-format.")
else()
    message(STATUS "CppServer/main found clang-format at ${CLANG_FORMAT_BIN}")
endif()

############################################################
# make tools
############################################################
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
############################################################
# compiler setup
############################################################
### compiler flags
# -fPIC: generate Position Independent Code
# -stdlib=libc++: use libc++ instead of libstdc++
# -Wall: enable all compiler warnings
# -Wextra: enable extra compiler warnings which is more strict than `-Wall`
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra -pthread")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -stdlib=libc++ -Wall -Wextra -pthread")
# -O0: disable compiler optimizations
# -ggdb: generate debugging info for the GDB debugger
# -fsanitize=address: enable AddressSanitizer, a tool for detecting memory errors
# -fno-optimize-sibling-calls: disables sibling call optimization, also helpfull for debugging
# BUG(wzy) disable the ASan, cuz it is not compatible with libc++ and gtest, any throw now will cause ASan error
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls")

# ### linker flags
# ### -Wl,-rpath,/usr/lib/llvm-17/lib: set the runtime library search path to /usr/lib/llvm-17/lib
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++ -lc++abi -Wl,-rpath,/usr/lib/llvm-17/lib")
# ### Or you can write like this
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++ -lc++ -lc++abi)

# ### enable ASan support for libc++
# add_compile_definitions(_LIBCPP_HAS_ASAN)

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
# include_directories(SYSTEM /usr/lib/llvm-17/include/c++/v1)
# link_directories(/usr/lib/llvm-17/lib)

# add sub CMakeLists.txt
add_subdirectory(src)
add_subdirectory(test)
