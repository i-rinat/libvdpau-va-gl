project (libvdpau-va-gl)
cmake_minimum_required (VERSION 2.6)
add_definitions(-std=gnu99 -Wall -fvisibility=hidden -ggdb)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SOMELIBS vdpau glib-2.0 libswscale libva-glx gl glu REQUIRED)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
enable_testing()
add_subdirectory(tests)

link_directories (
	${SOMELIBS_LIBRARY_DIRS}
)

include_directories (
	${SOMELIBS_INCLUDE_DIRS}
)

add_library (vdpau_va_gl SHARED
	vdpau-soft.c
	vdpau-entry.c
	vdpau-trace.c
	vdpau-locking.c
	reverse-constant.c
	handle-storage.c
	bitstream.c
	h264-parse.c
	globals.c
	watermark.c
)

add_library (xinitthreads SHARED xinitthreads.c)

target_link_libraries (vdpau_va_gl
	${SOMELIBS_LIBRARIES}
)

target_link_libraries (xinitthreads -lpthread -lX11)

set_target_properties (vdpau_va_gl
	PROPERTIES
	VERSION 1
)

install (TARGETS vdpau_va_gl DESTINATION lib)
