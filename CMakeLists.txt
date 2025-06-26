cmake_minimum_required(VERSION 3.20)
project(mxdbg VERSION 1.0.0 LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
endif()


if(NOT CMAKE_PREFIX_PATH)
set(CMAKE_PREFIX_PATH /usr/local)
endif()

set(CMAKE_INSTALL_RPATH ${CMAKE_PREFIX_PATH}/lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(libmxdbg)
add_subdirectory(mxdbg)
add_subdirectory(tests)


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    