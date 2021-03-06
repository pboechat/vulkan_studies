cmake_minimum_required(VERSION 3.0)

project(vulkan_studies)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message(SEND_ERROR "In-source builds are not allowed.")
endif ()

set(HOME ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")
set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/libs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBS})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIBS})

foreach(CMAKE_CONFIGURATION_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CMAKE_CONFIGURATION_TYPE} CMAKE_CONFIGURATION_TYPE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CMAKE_CONFIGURATION_TYPE} ${BIN})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CMAKE_CONFIGURATION_TYPE} ${LIBS})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CMAKE_CONFIGURATION_TYPE} ${LIBS})
endforeach(CMAKE_CONFIGURATION_TYPE CMAKE_CONFIGURATION_TYPES)

add_subdirectory(framework)
add_subdirectory(sample)