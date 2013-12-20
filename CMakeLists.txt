project (libvdpau-va-gl)
cmake_minimum_required (VERSION 2.8.8)
add_definitions(-std=gnu99 -Wall -fvisibility=hidden -fPIC)

find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)
pkg_check_modules(VDPAU      vdpau      REQUIRED)
pkg_check_modules(GLIB20     glib-2.0   REQUIRED)
pkg_check_modules(LIBVA      libva-glx  REQUIRED)
pkg_check_modules(LIBSWSCALE libswscale REQUIRED)
pkg_check_modules(GL         gl glu     REQUIRED)

set(DRIVER_NAME "vdpau_va_gl" CACHE STRING "driver name")
set(LIB_SUFFIX "" CACHE STRING "library path suffix (if needed)")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/vdpau" CACHE PATH "library installation path")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
add_custom_target(build-tests)
add_dependencies(check build-tests)
enable_testing()
add_subdirectory(tests)
add_subdirectory(glsl)

link_directories (
	${X11_LIBRARY_DIRS}
	${GLIB20_LIBRARY_DIRS}
	${LIBVA_LIBRARY_DIRS}
	${LIBSWSCALE_LIBRARY_DIRS}
	${GL_LIBRARY_DIRS}
)

include_directories (
	${X11_INCLUDE_DIRS}
	${VDPAU_INCLUDE_DIRS}
	${GLIB20_INCLUDE_DIRS}
	${LIBVA_INCLUDE_DIRS}
	${LIBSWSCALE_INCLUDE_DIRS}
	${GL_INCLUDE_DIRS}
	${GENERATED_INCLUDE_DIRS}
)

add_library (${DRIVER_NAME}_obj OBJECT
	vdpau-decoder.c
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

add_dependencies(${DRIVER_NAME}_obj shader-bundle)

add_library(${DRIVER_NAME} SHARED $<TARGET_OBJECTS:${DRIVER_NAME}_obj>)
add_library(${DRIVER_NAME}_static STATIC $<TARGET_OBJECTS:${DRIVER_NAME}_obj>)

set(link_libraries
	-lX11
	${GLIB20_LIBRARIES}
	${LIBVA_LIBRARIES}
	${LIBSWSCALE_LIBRARIES}
	${GL_LIBRARIES}
	-lpthread
	shader-bundle
)

target_link_libraries (${DRIVER_NAME} ${link_libraries})
target_link_libraries (${DRIVER_NAME}_static ${link_libraries})

# add_library (xinitthreads SHARED xinitthreads.c)
# target_link_libraries (xinitthreads -lpthread -lX11)

set_target_properties (${DRIVER_NAME}
	PROPERTIES
	VERSION 1
)

install (TARGETS ${DRIVER_NAME} DESTINATION ${LIB_INSTALL_DIR})
