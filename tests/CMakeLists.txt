find_package(ollama_gen REQUIRED)
add_executable(process_continue_test
    process_continue.cpp
)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

target_link_libraries(process_continue_test PRIVATE
    libmxdbg
    ollama_gen
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
    ollama_gen
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
    ollama_gen
)
target_include_directories(pipe_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(pipe_test PRIVATE cxx_std_20)
enable_language(C)
add_executable(pipe_write_test
    pipe_write.cpp
)
target_link_libraries(pipe_write_test PRIVATE
    libmxdbg
    ollama_gen
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
    ollama_gen
)
target_include_directories(exception_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(exception_test PRIVATE cxx_std_20)
add_executable(register_test
    register_test.cpp
)
target_include_directories(register_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(register_test PRIVATE cxx_std_20)
target_link_libraries(register_test PRIVATE
       libmxdbg
       ollama_gen
)
target_link_libraries(register_test PRIVATE
       libmxdbg
       ollama_gen
)
add_executable(c_test
    test_explain.c
)
set_target_properties(c_test PROPERTIES
    LINK_FLAGS "-no-pie"
)
add_executable(thread_test
    thread_test.c
)
set_target_properties(thread_test PROPERTIES
    LINK_FLAGS "-no-pie"
)
target_include_directories(c_test PRIVATE                         
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmxdbg/include
)
target_compile_features(c_test PRIVATE cxx_std_20)
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
enable_language(ASM)
add_executable(hello_world asm/hello_world.s)
set_target_properties(hello_world PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-nostdlib -static -no-pie"
)
add_executable(registers_test_asm asm/registers_test.s)
set_target_properties(registers_test_asm PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-nostdlib -static -no-pie"
)
add_executable(program_test_asm asm/test_program.s)
set_target_properties(program_test_asm PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-nostdlib -static -no-pie"
)
add_executable(program_linked_list_asm asm/linked_list.s)
set_target_properties(program_linked_list_asm PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-nostdlib -static -no-pie"
)
add_executable(program_file_asm asm/file_test.s)
set_target_properties(program_file_asm PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-nostdlib -static -no-pie"
)
add_executable(program_array_asm asm/array.s)
set_target_properties(program_array_asm PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)
add_executable(program_t asm/program.s)
set_target_properties(program_t PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)
add_executable(program_input asm/get_input.s)
set_target_properties(program_input PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)
add_executable(program_string asm/get_string.s)
set_target_properties(program_string PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)
add_executable(program_cloop asm/copy_loop.s)
set_target_properties(program_cloop PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)
add_executable(program_cat asm/string_cat.s asm/string_copy.s copy_string.c)
set_target_properties(program_cat PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_print asm/print_loop.s asm/echo_for.s asm/echo_backward.s print_loop.c)
set_target_properties(program_print PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_file asm/file.s)
set_target_properties(program_file PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_setxy asm/setxy.s program.c)
set_target_properties(program_setxy PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_trim asm/trim_com.s trim_com.c)
set_target_properties(program_trim PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_trim_file asm/trim_file.s trim_file.c)
set_target_properties(program_trim_file PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_loopbyx asm/loopbyx.s)
set_target_properties(program_loopbyx PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(print_hex asm/print_hex.s)
set_target_properties(print_hex PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(print_hex_c print_hex.c)
set_target_properties(print_hex PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(print_hex_c_do print_hex_do.c)
set_target_properties(print_hex_c_do PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(echo_c echo.c)
set_target_properties(echo_c PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(echo asm/echo.s)
set_target_properties(echo PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(l_list asm/list.s)
set_target_properties(l_list PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_stack_numbers asm/stack_numbers.s)
set_target_properties(program_stack_numbers PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_float asm/float.s)
set_target_properties(program_float PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_double asm/double.s)
set_target_properties(program_double PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)

add_executable(program_prompt asm/prompt.s)
set_target_properties(program_prompt PROPERTIES
    LINKER_LANGUAGE ASM
    LINK_FLAGS "-no-pie"
)