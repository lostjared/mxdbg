
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

add_executable(pipe_test
    pipe_test.cpp
)
target_link_libraries(pipe_test PRIVATE
    libmxdbg
)
target_include_directories(pipe_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(pipe_test PRIVATE cxx_std_20)


add_executable(pipe_write_test
    pipe_write.cpp
)
target_link_libraries(pipe_write_test PRIVATE
    libmxdbg
)
target_include_directories(pipe_write_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(pipe_write_test PRIVATE cxx_std_20)

add_executable(exception_test
    exception_test.cpp
)
target_link_libraries(exception_test PRIVATE
    libmxdbg
)
target_include_directories(exception_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(exception_test PRIVATE cxx_std_20)

add_executable(register_test
    register_test.cpp
)

target_link_libraries(register_test PRIVATE
    libmxdbg
)

enable_testing()

add_test(NAME process_continue COMMAND process_continue_test)
add_test(NAME process_attach  COMMAND process_attach_test)
add_test(NAME pipe_test_  COMMAND pipe_test)
add_test(NAME pipe_write COMMAND pipe_write_test)
add_test(NAME exception_test_  COMMAND exception_test)
add_test(NAME register_test COMMAND register_test)


set_tests_properties(process_attach PROPERTIES
    TIMEOUT 5
    PASS_REGULAR_EXPRESSION "Test passed"
)