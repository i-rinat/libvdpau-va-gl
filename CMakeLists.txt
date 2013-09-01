project (libvdpau-va-gl)
cmake_minimum_required (VERSION 2.6)
add_definitions(-std=gnu99 -Wall -fvisibility=hidden)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SOMELIBS vdpau glib-2.0 libswscale libva-glx gl glu REQUIRED)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
add_custom_target(build-tests)
add_dependencies(check build-tests)
enable_testing()
add_subdirectory(tests)

link_directories (
	${SOMELIBS_LIBRARY_DIRS}
)

include_directories (
	${SOMELIBS_INCLUDE_DIRS}
)

set(DRIVER_NAME "vdpau_va_gl" CACHE STRING "driver name")
set(LIB_SUFFIX "" CACHE STRING "library path suffix (if needed)")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/vdpau" CACHE PATH "library installation path")

add_library (${DRIVER_NAME} SHARED
	vdpau-soft.c
	vdpau-presentation.c
	vdpau-entry.c
	vdpau-trace.c
	reverse-constant.c
	handle-storage.c
	bitstream.c
	h264-parse.c
	globals.c
	watermark.c
	ctx-stack.c
)

add_library (xinitthreads SHARED xinitthreads.c)

target_link_libraries (${DRIVER_NAME}
	${SOMELIBS_LIBRARIES}
)

target_link_libraries (xinitthreads -lpthread -lX11)

set_target_properties (${DRIVER_NAME}
	PROPERTIES
	VERSION 1
)

install (TARGETS ${DRIVER_NAME} DESTINATION ${LIB_INSTALL_DIR})
