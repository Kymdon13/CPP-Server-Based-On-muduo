file(GLOB CPPSERVER_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*.cc" "${PROJECT_SOURCE_DIR}/test/**/*.cc")

# for seccomp
add_link_options(-lseccomp)

########################################
# gRPC
########################################
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
find_package(Threads REQUIRED)
option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
# TARGET_FILE will return the full path to the target executable protobuf::protoc
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
# TARGET_FILE will return the full path to the target executable gRPC::grpc_cpp_plugin
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(exe_proto "../protos/executor.proto" ABSOLUTE)
get_filename_component(exe_proto_path "${exe_proto}" PATH)

# Generated sources
set(exe_proto_srcs "${CMAKE_BINARY_DIR}/executor.pb.cc")
set(exe_proto_hdrs "${CMAKE_BINARY_DIR}/executor.pb.h")
set(exe_grpc_srcs "${CMAKE_BINARY_DIR}/executor.grpc.pb.cc")
set(exe_grpc_hdrs "${CMAKE_BINARY_DIR}/executor.grpc.pb.h")
add_custom_command(
    OUTPUT "${exe_proto_srcs}" "${exe_proto_hdrs}" "${exe_grpc_srcs}" "${exe_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_BINARY_DIR}"
    --cpp_out "${CMAKE_BINARY_DIR}"
    -I "${exe_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${exe_proto}"
    DEPENDS "${exe_proto}")

# exe_grpc_proto
add_library(exe_grpc_proto STATIC
    ${exe_grpc_srcs}
    ${exe_grpc_hdrs}
    ${exe_proto_srcs}
    ${exe_proto_hdrs})
target_include_directories(exe_grpc_proto PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(exe_grpc_proto
    absl::check
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
########################################
# gRPC Done
########################################

####################
# make all executables
####################
foreach(cppserver_test_source ${CPPSERVER_TEST_SOURCES})
    # get name of each cppserver_test_source and store it into ${cppserver_test_filename}
    get_filename_component(cppserver_test_filename ${cppserver_test_source} NAME)
    # remove suffix and store it into cppserver_test_name
    string(REPLACE ".cc" "" cppserver_test_name ${cppserver_test_filename})

    # add the test target
    add_executable(${cppserver_test_name} EXCLUDE_FROM_ALL ${cppserver_test_source})

    # CPPSERVER is defined in src/CMakeLists.txt
    target_link_libraries(${cppserver_test_name} CPPSERVER)
    target_link_libraries(${cppserver_test_name}
        exe_grpc_proto
        absl::check
        absl::flags
        absl::flags_parse
        absl::log
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})
endforeach(cppserver_test_source ${CPPSERVER_TEST_SOURCES})
