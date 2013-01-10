project (fake-vdpau)
cmake_minimum_required (VERSION 2.6)
add_definitions(-std=c99 -Wall -fvisibility=hidden -ggdb)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VDPAU vdpau REQUIRED)
pkg_check_modules(GLIB20 glib-2.0 REQUIRED)
pkg_check_modules(SWSCALE libswscale REQUIRED)

link_directories (
	${VDPAU_LIBRARY_DIRS}
	${GLIB20_LIBRARY_DIRS}
	${STATIC_SWSCALE_LIBRARY_DIRS}
)

include_directories (
	${VDPAU_INCLUDE_DIRS}
	${GLIB20_INCLUDE_DIRS}
	${SWSCALE_INCLUDE_DIRS}
)

add_library (vdpau_fake SHARED
	vdpau_fake.c
	reverse-constant.c
	handle-storage.c
)

target_link_libraries (vdpau_fake
	${GLIB20_LIBRARIES}
	${STATIC_SWSCALE_LIBRARIES}
)

set_target_properties (vdpau_fake
	PROPERTIES
	SOVERSION 1
)
