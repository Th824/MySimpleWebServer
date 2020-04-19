cmake_minimum_required(VERSION 2.6)

project(SimpleServer LANGUAGES CXX)

# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE "Debug")
# endif()

# 添加 option
option(BUILD_APPLICATIONS "Build Appliacations" ON)
option(RUN_TEST "Run Test" ON)

# 设置变量
set(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DRTSP_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# todo:for gtest
# find_library(GOOGLETEST_LIBRARY NAMES gtest)

# include file -L 
include_directories(${PROJECT_SOURCE_DIR})

# 添加子文件夹，每个子文件夹会有对应的 cmakefile
add_subdirectory(${PROJECT_SOURCE_DIR}/base)
add_subdirectory(${PROJECT_SOURCE_DIR}/net)
# add_subdirectory(${PROJECT_SOURCE_DIR}/ideal/media)

if(BUILD_APPLICATIONS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/application/http_server)
endif()

if(RUN_TEST)
    # add_subdirectory(${PROJECT_SOURCE_DIR}/application/http_server)
endif()

