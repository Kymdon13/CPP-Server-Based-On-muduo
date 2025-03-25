# store all source file in a list
file(GLOB CPPSERVER_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)
# add all the source file into a shared lib, cmake will gen a lib<LIB_NAME>.so in build/lib
add_library(CPPSERVER_SHARED SHARED ${CPPSERVER_SOURCES})
