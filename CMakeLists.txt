cmake_minimum_required(VERSION 3.9)
MESSAGE(STATUS "CMAKE_ROOT: " ${CMAKE_ROOT})

project(DelayPredict)

# Build in 64bits
set(BUILD_USE_64BITS on)

# Turn on the verbose
set(CMAKE_VERBOSE_MAKEFILE ON)

# CXX target properties
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(USE_WERROR ON)

set(CMAKE_BUILD_TYPE Release)

find_package(OpenMP REQUIRED)

set(CMAKE_CXX_FLAGS "-g -std=c++17 -lstdc++fs ${CMAKE_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS"${CMAKE_EXE_LINKER_FLAGS}")

#
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/Parser-SPEF/parser-spef)
include_directories(${PROJECT_SOURCE_DIR}/eigen-3.4.0/Eigen)
include_directories(Tree)
include_directories(Read_train)
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

#
add_executable(course Tree/course.cpp Read_train/Read.cpp)
target_link_libraries(course stdc++fs OpenMP::OpenMP_CXX)

add_executable(parser_test Tree/parser_test.cpp)
target_link_libraries(parser_test stdc++fs)

include_directories(${PROJECT_SOURCE_DIR}/boost_1_90_0/boost)
