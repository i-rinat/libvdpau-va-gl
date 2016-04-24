include_directories(
    ${GENERATED_INCLUDE_DIRS}
)

link_directories (
    ${X11_LIBRARY_DIRS}
    ${LIBVA_LIBRARY_DIRS}
    ${LIBGL_LIBRARY_DIRS}
)

add_library(${DRIVER_NAME} SHARED
    api-bitmap-surface.cc
    api-csc-matrix.cc
    api-decoder.cc
    api-device.cc
    api-output-surface.cc
    api-presentation-queue.cc
    api-video-mixer.cc
    api-video-surface.cc
    entry.cc
    globals.cc
    glx-context.cc
    h264-parse.cc
    handle-storage.cc
    reverse-constant.cc
    trace.cc
    watermark.cc
    x-display-ref.cc
)

add_dependencies(${DRIVER_NAME} shader-bundle)

set(LINK_LIBRARIES
    ${X11_LIBRARIES}
    ${LIBVA_LIBRARIES}
    ${LIBGL_LIBRARIES}
    -lrt
    shader-bundle
)

target_link_libraries(${DRIVER_NAME} ${LINK_LIBRARIES} ${SYMBOLMAP})

set_target_properties(${DRIVER_NAME}
    PROPERTIES
    VERSION 1
)

install(TARGETS ${DRIVER_NAME} DESTINATION ${LIB_INSTALL_DIR})
