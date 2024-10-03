cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

add_executable(libresplit_startup_tests_exe startup_tests.c)

add_test(NAME libresplit_startup_tests COMMAND libresplit_startup_tests_exe)

set_tests_properties(libresplit_startup_tests PROPERTIES PASS 0)
