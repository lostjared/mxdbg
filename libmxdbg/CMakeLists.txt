add_library(libmxdbg SHARED
    process.cpp
)

set_target_properties(libmxdbg PROPERTIES
    OUTPUT_NAME mxdbg
)


target_include_directories(libmxdbg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(libmxdbg PUBLIC cxx_std_20)