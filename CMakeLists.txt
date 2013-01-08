project (fake-vdpau)

cmake_minimum_required (VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VDPAU vdpau REQUIRED)
pkg_check_modules(GLIB20 glib-2.0 REQUIRED)

link_directories (
	${VDPAU_LIBRARY_DIRS}
	${GLIB20_LIBRARY_DIRS}
)

include_directories (
	${VDPAU_INCLUDE_DIRS}
	${GLIB20_INCLUDE_DIRS}
)

add_library (vdpau_fake SHARED
	vdpau_fake.c
	reverse-constant.c
	handle-storage.c
)

target_link_libraries (vdpau_fake
	${GLIB20_LIBRARIES}
)

set_target_properties (vdpau_fake
	PROPERTIES
	SOVERSION 1
)
