cmake_minimum_required(VERSION 2.8.12...4.0)
project (libvdpau-va-gl)
include(GNUInstallDirs)
add_compile_options(-Wall -fvisibility=hidden -fPIC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")

find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)
pkg_check_modules(LIBVA      libva-x11  REQUIRED)
pkg_check_modules(LIBGL      gl         REQUIRED)

set(DRIVER_NAME "vdpau_va_gl" CACHE STRING "driver name")

include_directories (
    3rdparty
    ${X11_INCLUDE_DIRS}
    ${LIBVA_INCLUDE_DIRS}
    ${LIBGL_INCLUDE_DIRS}
    ${GENERATED_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

# filter public symbols
set(SYMBOLMAP "-Wl,-version-script=\"${CMAKE_SOURCE_DIR}/src/symbolmap\"")

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
add_custom_target(build-tests)
add_dependencies(check build-tests)
add_subdirectory(glsl)
enable_testing()
add_subdirectory(tests)

# put compiled library to the build directory root
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(src)
