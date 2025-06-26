add_library(libmxdbg SHARED
    process.cpp 
    pipe.cpp
    debugger.cpp
    scanner.cpp
    string_buffer.cpp
    types.cpp
    exception.cpp
)

set_target_properties(libmxdbg PROPERTIES
    OUTPUT_NAME mxdbg
)

find_package(ollama_gen REQUIRED)


target_include_directories(libmxdbg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OLLAMA_GEN_INCLUDE_DIRS}
)
target_link_libraries(libmxdbg PUBLIC ${OLLAMA_GEN_LIBRARIES})
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

