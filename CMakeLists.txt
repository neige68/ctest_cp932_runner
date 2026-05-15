# CMakeLists.txt
#
# ctest_cp932_runner
# Windows の CTest で CP932 出力するテストプログラムを UTF-8 に変換するランナー

cmake_minimum_required(VERSION 3.20)

project(ctest_cp932_runner CXX)

if(NOT WIN32)
    message(FATAL_ERROR "cp932_test_runner は Windows 専用です")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cp932_test_runner cp932_test_runner.cpp)

if(MSVC)
    target_compile_options(cp932_test_runner PRIVATE /W3 /EHsc /utf-8)
endif()

install(TARGETS cp932_test_runner RUNTIME DESTINATION bin)

if(PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(test)
endif()
