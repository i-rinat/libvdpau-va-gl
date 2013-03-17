cmake_minimum_required(VERSION 2.8)
#project(tests-for-libvdpau-va-gl)

include_directories(..)

add_executable(rbsp-test EXCLUDE_FROM_ALL rbsp-test.c ../bitstream.c)
add_executable(test-001 EXCLUDE_FROM_ALL test-001.c)

add_test(rbsp-test ${CMAKE_CURRENT_BINARY_DIR}/rbsp-test)
add_test(test-001 ${CMAKE_CURRENT_BINARY_DIR}/test-001)

add_dependencies(check rbsp-test)
add_dependencies(check test-001)
