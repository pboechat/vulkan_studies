cmake_minimum_required(VERSION 3.0)

project(vkfw C CXX)

find_package(Vulkan REQUIRED)
if(LINUX) 
    find_package(X11 REQUIRED)
endif()

file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/include/vkfw/*.h" "${PROJECT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

include_directories(
	${Vulkan_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/include
)
if(LINUX)
	include_directories(
		${X11_INCLUDE_DIR}
	)
endif()

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include 
	CACHE INTERNAL "${PROJECT_NAME}: includes" FORCE)