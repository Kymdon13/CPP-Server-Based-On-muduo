file(GLOB CPPSERVER_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*.cc" "${PROJECT_SOURCE_DIR}/test/**/*.cc")

####################
# `make build-test`
# `make check-test`
####################
# FIXME(ban)
# add_custom_target(build-tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)
# add_custom_target(check-tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

####################
# make all executables
####################
foreach (cppserver_test_source ${CPPSERVER_TEST_SOURCES})
    # get name of each cppserver_test_source and store it into ${cppserver_test_filename}
    get_filename_component(cppserver_test_filename ${cppserver_test_source} NAME)
    # remove suffix and store it into cppserver_test_name
    string(REPLACE ".cc" "" cppserver_test_name ${cppserver_test_filename})

    # add the test target
    add_executable(${cppserver_test_name} EXCLUDE_FROM_ALL ${cppserver_test_source})
    # add the test target as part of `make check-test`
    # FIXME(ban)
    # add_dependencies(build-tests ${cppserver_test_name})
    # add_dependencies(check-tests ${cppserver_test_name})

    # CPPSERVER is defined in src/CMakeLists.txt
    target_link_libraries(${cppserver_test_name} CPPSERVER)

    # set test target properties and dependencies
    set_target_properties(${cppserver_test_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" # build/bin
        COMMAND ${cppserver_test_name}
    )
endforeach(cppserver_test_source ${CPPSERVER_TEST_SOURCES})
