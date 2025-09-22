cmake_minimum_required(VERSION 3.0...3.25)
project(
	FixedSizePoints
	LANGUAGES CXX C
)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

add_executable(FixedSizePoints
	main.cpp
	ShaderLoader.cpp
)

target_link_libraries(FixedSizePoints PRIVATE glfw webgpu glfw3webgpu)

set_target_properties(FixedSizePoints PROPERTIES
	CXX_STANDARD 23 
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	COMPILE_WARNING_AS_ERROR ON
)

target_compile_options(FixedSizePoints PRIVATE -Wall -Wextra -pedantic)

add_custom_target(
	ShaderCopy
	${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/PointCloud.wgsl ${CMAKE_BINARY_DIR}
	COMMENT "Copying shader to binary directory"
)

add_dependencies(
	FixedSizePoints
	ShaderCopy
)
