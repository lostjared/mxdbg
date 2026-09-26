add_executable(mxdbg
    main.cpp
)
if(NOT TARGET ollama_gen)
    find_package(ollama_gen REQUIRED)
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)
pkg_check_modules(LIBCURL REQUIRED IMPORTED_TARGET libcurl)

target_link_libraries(mxdbg PRIVATE
    libmxdbg
    ${READLINE_LIBRARIES}
    ollama_gen
    PkgConfig::LIBCURL
)
target_include_directories(mxdbg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${READLINE_INCLUDE_DIRS}
)

target_compile_features(mxdbg PRIVATE cxx_std_20)
target_compile_options(mxdbg PRIVATE ${READLINE_CFLAGS_OTHER})

install(TARGETS mxdbg DESTINATION bin)
