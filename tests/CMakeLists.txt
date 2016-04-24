include_directories(..)
include_directories(${GENERATED_INCLUDE_DIRS})

add_definitions(-DDRIVER_NAME="${CMAKE_BINARY_DIR}/lib${DRIVER_NAME}.so.1")

link_libraries(${X11_LIBRARIES})
link_directories(${X11_LIBRARY_DIRS})

list(APPEND _vdpau_tests
    test-001 test-002 test-003 test-004 test-005 test-006
    test-007 test-008 test-009 test-010)

list(APPEND _all_tests test-000 ${_vdpau_tests})

add_executable(test-000 EXCLUDE_FROM_ALL test-000.cc)

foreach(_test ${_vdpau_tests})
    add_executable(${_test} EXCLUDE_FROM_ALL "${_test}.c" tests-common.c)
    add_dependencies(${_test} ${DRIVER_NAME})
    target_link_libraries(${_test} dl)
endforeach(_test)

foreach(_test ${_all_tests})
    add_test(${_test} ${CMAKE_CURRENT_BINARY_DIR}/${_test})
    add_dependencies(build-tests ${_test})
endforeach(_test)

# tmp for testing

add_executable(conv-speed EXCLUDE_FROM_ALL conv-speed.c)
target_link_libraries(conv-speed ${DRIVER_NAME}_static)
