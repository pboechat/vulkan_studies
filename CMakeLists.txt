cmake_minimum_required(VERSION 3.8)

project(vulkan_studies)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message(SEND_ERROR "In-source builds are not allowed.")
endif ()

set(HOME ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")
set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/libs)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND LINUX)
	add_definitions(-D_DEBUG=1)
endif()

add_subdirectory(vkfw)
add_subdirectory(samples)
