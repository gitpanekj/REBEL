cmake_minimum_required(VERSION 3.20.0)
project(rebel)

set(CMAKE_CXX_STANDARD 23 CACHE STRING "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(LT_LLVM_INSTALL_DIR "" CACHE PATH "")

add_subdirectory(cpp_modules_src)

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    include(cmake/FetchDependencies.cmake)
    add_subdirectory(tests)
endif()
