cmake_minimum_required(VERSION 3.20)
project(mxdbg VERSION 1.3.0 LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OLLAMA_GEN_SOURCE_DIR "" CACHE PATH "Path to an ollama_gen source checkout")
if(OLLAMA_GEN_SOURCE_DIR)
    if(NOT EXISTS "${OLLAMA_GEN_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "OLLAMA_GEN_SOURCE_DIR does not contain CMakeLists.txt: ${OLLAMA_GEN_SOURCE_DIR}")
    endif()
    add_subdirectory("${OLLAMA_GEN_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/ollama_gen" EXCLUDE_FROM_ALL)
    # libmxdbg is shared, so a source-built static dependency must be PIC.
    set_target_properties(ollama_gen PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

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
option(TESTS "Build tests" ON)
if(TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
