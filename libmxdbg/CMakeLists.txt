add_library(libmxdbg SHARED
    process.cpp 
    pipe.cpp
    debugger.cpp
    debug_context.cpp
    scanner.cpp
    string_buffer.cpp
    types.cpp
    exception.cpp
)

set_target_properties(libmxdbg PROPERTIES
    OUTPUT_NAME mxdbg
)

if(NOT TARGET ollama_gen)
    find_package(ollama_gen REQUIRED)
endif()


target_include_directories(libmxdbg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(libmxdbg PUBLIC ollama_gen)
target_compile_features(libmxdbg PUBLIC cxx_std_20)

include(GNUInstallDirs)

install(TARGETS libmxdbg
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/mxdbg
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)
