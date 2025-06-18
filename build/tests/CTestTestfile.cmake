# CMake generated Testfile for 
# Source directory: /home/jared/dbg/mxdbg/tests
# Build directory: /home/jared/dbg/mxdbg/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[process_continue]=] "/home/jared/dbg/mxdbg/build/tests/process_continue_test")
set_tests_properties([=[process_continue]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/jared/dbg/mxdbg/tests/CMakeLists.txt;48;add_test;/home/jared/dbg/mxdbg/tests/CMakeLists.txt;0;")
add_test([=[process_attach]=] "/home/jared/dbg/mxdbg/build/tests/process_attach_test")
set_tests_properties([=[process_attach]=] PROPERTIES  PASS_REGULAR_EXPRESSION "Test passed" TIMEOUT "5" _BACKTRACE_TRIPLES "/home/jared/dbg/mxdbg/tests/CMakeLists.txt;49;add_test;/home/jared/dbg/mxdbg/tests/CMakeLists.txt;0;")
add_test([=[pipe_test_]=] "/home/jared/dbg/mxdbg/build/tests/pipe_test")
set_tests_properties([=[pipe_test_]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/jared/dbg/mxdbg/tests/CMakeLists.txt;50;add_test;/home/jared/dbg/mxdbg/tests/CMakeLists.txt;0;")
add_test([=[exception_test_]=] "/home/jared/dbg/mxdbg/build/tests/exception_test")
set_tests_properties([=[exception_test_]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/jared/dbg/mxdbg/tests/CMakeLists.txt;51;add_test;/home/jared/dbg/mxdbg/tests/CMakeLists.txt;0;")
