# test/CMakeLists.txt
#
# Copyright (c) 2026 neige68
# https://github.com/neige68/ctest_cp932_runner
# SPDX-License-Identifier: MIT
#
# cp932_test_runner のテスト

add_executable(helper_cp932_output helper_cp932_output.cpp)
add_executable(helper_exit_nonzero helper_exit_nonzero.cpp)

if(MSVC)
    target_compile_options(helper_cp932_output PRIVATE /W3 /EHsc /utf-8)
endif()

# テスト 1: CP932 stdout/stderr が UTF-8 に変換されること
# --codepage 932 を明示することで非日本語 Windows 環境でも動作する
add_test(NAME test_conversion
    COMMAND "$<TARGET_FILE:cp932_test_runner>" --codepage 932 "$<TARGET_FILE:helper_cp932_output>"
)
set_tests_properties(test_conversion PROPERTIES
    PASS_REGULAR_EXPRESSION "日本語テスト出力"
)

# テスト 2: 非ゼロ終了コードが pass-through されること
add_test(NAME test_exit_nonzero
    COMMAND "$<TARGET_FILE:cp932_test_runner>" "$<TARGET_FILE:helper_exit_nonzero>"
)
set_tests_properties(test_exit_nonzero PROPERTIES
    WILL_FAIL TRUE
)
