set(shader_list_no_path
	NV12_RGBA.glsl
	YV12_RGBA.glsl
	red_to_alpha_swizzle.glsl
)
set(GENERATED_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

set(shader_list)
foreach(item ${shader_list_no_path})
	list(APPEND shader_list ${CMAKE_CURRENT_SOURCE_DIR}/${item})
endforeach(item)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.c ${CMAKE_CURRENT_BINARY_DIR}/shaders.h
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shader-bundle-tool ${CMAKE_CURRENT_BINARY_DIR}/shaders.h ${CMAKE_CURRENT_BINARY_DIR}/shaders.c ${shader_list}
	DEPENDS ${shader_list} shader-bundle-tool
)
add_custom_target(shader-bundle-src DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders.c ${CMAKE_CURRENT_BINARY_DIR}/shaders.h)

add_executable(shader-bundle-tool shader-bundle-tool.c)
add_library(shader-bundle STATIC ${CMAKE_CURRENT_BINARY_DIR}/shaders.c)
add_dependencies(shader-bundle shader-bundle-src)
