cmake_minimum_required(VERSION 3.20)
project(mxdbg VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libmxdbg)
add_subdirectory(mxdbg)