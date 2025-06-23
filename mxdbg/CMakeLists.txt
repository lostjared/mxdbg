add_executable(mxdbg
    main.cpp
)
find_package(ollama_gen REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)

set(OLLAMA_GEN_LIBRARIES ollama_gen)


target_link_libraries(mxdbg PRIVATE
    libmxdbg
    ${READLINE_LIBRARIES}
    ${OLLAMA_GEN_LIBRARIES}
)
target_include_directories(mxdbg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${READLINE_INCLUDE_DIRS}
    ${OLLAMA_GEN_INCLUDE_DIRS}
)

target_compile_features(mxdbg PRIVATE cxx_std_20)
target_compile_options(mxdbg PRIVATE ${READLINE_CFLAGS_OTHER})

install(TARGETS mxdbg DESTINATION bin)