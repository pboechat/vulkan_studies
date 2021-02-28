cmake_minimum_required(VERSION 3.0)

project(sample C CXX)

find_package(Vulkan REQUIRED)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

include_directories(
	${Vulkan_INCLUDE_DIRS}
	${framework_INCLUDE_DIRS}
)

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME} framework ${Vulkan_LIBRARIES})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include 
	CACHE INTERNAL "${PROJECT_NAME}: includes" FORCE)

