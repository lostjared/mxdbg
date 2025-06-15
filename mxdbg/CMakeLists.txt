
add_executable(mxdbg
    main.cpp
)

target_link_libraries(mxdbg PRIVATE
    libmxdbg
)

target_include_directories(mxdbg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(mxdbg PRIVATE cxx_std_20)