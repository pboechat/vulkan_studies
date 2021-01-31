cmake_minimum_required(VERSION 3.0)

project(framework C CXX)

find_package(Vulkan REQUIRED)

file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/include/framework/*.h" "${PROJECT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

include_directories(
	${Vulkan_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/include
)

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include 
	CACHE INTERNAL "${PROJECT_NAME}: includes" FORCE)