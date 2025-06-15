
add_executable(process_continue_test
    process_continue.cpp
)
target_link_libraries(process_continue_test PRIVATE
    libmxdbg
)
target_include_directories(process_continue_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(process_continue_test PRIVATE cxx_std_20)

add_executable(process_attach_test
    process_pid.cpp
)
target_link_libraries(process_attach_test PRIVATE
    libmxdbg
)
target_include_directories(process_attach_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(process_attach_test PRIVATE cxx_std_20)

enable_testing()

add_test(NAME process_continue COMMAND process_continue_test)
add_test(NAME process_attach  COMMAND process_attach_test)

set_tests_properties(process_attach PROPERTIES
    TIMEOUT 5
    PASS_REGULAR_EXPRESSION "Test passed"
)