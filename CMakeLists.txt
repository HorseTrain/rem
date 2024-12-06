cmake_minimum_required(VERSION 3.10)

set(PROJECT rem)
set(SOURCE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/)
set(LIBRARY_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/libraries/)

project(${PROJECT})

add_definitions(-std=c++17)
add_compile_definitions(XBYAK64)

set(CMAKE_BUILD_TYPE Debug)

file(GLOB_RECURSE SOURCES "${SOURCE_DIRECTORY}*.cpp")
file(GLOB_RECURSE HEADERS "${SOURCE_DIRECTORY}*.h")

include_directories(${PROJECT} ${LIBRARY_DIRECTORY})
include_directories(${PROJECT} ${SOURCE_DIRECTORY})

add_executable(${PROJECT} ${SOURCES} ${HEADERS})

set_property(TARGET ${PROJECT} PROPERTY CXX_STANDARD 17)