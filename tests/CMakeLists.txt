cmake_minimum_required(VERSION 2.8)
project(tests-for-libvdpau-va-gl)

include_directories(..)
include_directories(${GENERATED_INCLUDE_DIRS})
find_package(X11 REQUIRED)
pkg_check_modules(VDPAU vdpau REQUIRED)

link_libraries(${X11_LIBRARIES} ${VDPAU_LIBRARIES} -lpthread)
link_directories(${X11_LIBRARY_DIRS} ${VDPAU_LIBRARY_DIRS})

list(APPEND _vdpau_tests
	test-001 test-002 test-003 test-004 test-005 test-006
	test-007 test-008 test-009 test-010)

list(APPEND _all_tests test-000 ${_vdpau_tests})

add_executable(test-000 EXCLUDE_FROM_ALL test-000.c ../src/bitstream.c)

foreach(_test ${_vdpau_tests})
	add_executable(${_test} EXCLUDE_FROM_ALL "${_test}.c" vdpau-init.c)
endforeach(_test)

foreach(_test ${_all_tests})
	add_test(${_test} ${CMAKE_CURRENT_BINARY_DIR}/${_test})
	add_dependencies(build-tests ${_test})
endforeach(_test)

# tmp for testing

include_directories(${GLIB20_INCLUDE_DIRS})
add_executable(conv-speed conv-speed.c)
target_link_libraries(conv-speed ${DRIVER_NAME}_static)
