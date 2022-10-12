cmake_minimum_required(VERSION 3.8)

include(../../cmake/glsl.cmake)

project(obj_loader C CXX)

find_package(Vulkan REQUIRED)
if(LINUX) 
    find_package(X11 REQUIRED)
endif()

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")

include_directories(
	${Vulkan_INCLUDE_DIRS}
	${vkfw_INCLUDE_DIRS}
)

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})
source_group("shaders" FILES ${SHADERS})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES} ${SHADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BIN}/${PROJECT_NAME}/$<CONFIG>" POSITION_INDEPENDENT_CODE CXX LINKER_LANGUAGE CXX)
add_glsl(${PROJECT_NAME} ${SHADERS})
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN}/${PROJECT_NAME}")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/models" "${BIN}/${PROJECT_NAME}/$<CONFIG>/models")
target_link_libraries(${PROJECT_NAME} vkfw ${Vulkan_LIBRARIES})
if(LINUX)
	target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
endif()