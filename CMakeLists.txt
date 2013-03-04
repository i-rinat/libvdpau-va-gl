project (fake-vdpau)
cmake_minimum_required (VERSION 2.6)
add_definitions(-std=c99 -Wall -fvisibility=hidden -ggdb)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SOMELIBS vdpau glib-2.0 libswscale libva-glx gl glu REQUIRED)

link_directories (
	${SOMELIBS_LIBRARY_DIRS}
)

include_directories (
	${SOMELIBS_INCLUDE_DIRS}
)

add_library (vdpau_fake SHARED
	vdpau-soft.c
	vdpau-entry.c
	vdpau-trace.c
	reverse-constant.c
	handle-storage.c
	bitstream.c
)

target_link_libraries (vdpau_fake
	${SOMELIBS_LIBRARIES}
)

set_target_properties (vdpau_fake
	PROPERTIES
	VERSION 1
)

install (TARGETS vdpau_fake DESTINATION lib)
