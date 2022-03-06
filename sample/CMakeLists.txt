cmake_minimum_required(VERSION 3.0)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

project(sample C CXX)

find_package(Vulkan REQUIRED)
if(LINUX) 
    find_package(X11 REQUIRED)
endif()

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

include_directories(
	${Vulkan_INCLUDE_DIRS}
	${vkfw_INCLUDE_DIRS}
)

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME} vkfw ${Vulkan_LIBRARIES})
if(LINUX)
	target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
endif()

