# ctest_cp932_runner

> **Japanese:** [README.md](README.md) is also available.

A test runner wrapper for CTest on Windows that converts CP932 output to UTF-8.

---

## When You Need This

Use this tool if all of the following apply to your project:

- You are running CMake + CTest on **Windows**
- Your test executables output text encoded in **CP932 (Windows-31J / Shift_JIS)**
- The test output contains **Japanese text** that you want displayed correctly

CP932 output typically occurs in legacy Japanese software that has not yet been migrated to UTF-8.

---

## Why It Is Needed

### How CTest Handles Encoding

CTest (CMake 3.20+) processes test output as **UTF-8 byte sequences**. Valid UTF-8 sequences (including ASCII) pass through unchanged, but **byte sequences that are invalid UTF-8 are replaced with U+FFFD (`\xEF\xBF\xBD`)**. CP932 multibyte characters are invalid UTF-8, so Japanese text encoded in CP932 becomes garbled.

CP932 two-byte characters consist of a lead byte (0x81–0x9F or 0xE0–0xFC) and a trail byte (0x40–0x7E or 0x80–0xFC). Trail bytes in the range 0x40–0x7E fall within the ASCII range and pass through unchanged. The result is a mix of U+FFFD replacement characters and stray ASCII fragments.

Actual output when `stdout: 日本語テスト出力` is written in CP932:

```
Expected: stdout: 日本語テスト出力
Actual:   stdout: ���{��e�X�g�o��
```

(`?` = U+FFFD replacement character; `{`, `e`, `X`, `g`, `o` are CP932 trail bytes that passed through as ASCII)

### Why `cmd.exe /C` Does Not Work in `add_test`

Using `add_test(COMMAND cmd.exe /C ...)` fails. CTest **individually quotes** each argument in `COMMAND` before passing them to `CreateProcess`. This turns `/C` into `"/C"`, which cmd.exe does not recognize as a flag, producing:

```
The syntax of the command is incorrect.
```

### How This Tool Solves the Problem

`cp932_test_runner.exe` wraps the test process:

```
Test process (stdout/stderr: CP932)
  → cp932_test_runner captures via pipes (two threads reading concurrently)
  → MultiByteToWideChar(CP 932) + WideCharToMultiByte(CP_UTF8) conversion
  → WriteFile writes UTF-8 to its own stdout/stderr
  → CTest passes the UTF-8 bytes through unchanged
  → Japanese text is displayed correctly
```

Both stdout and stderr are read on separate threads to prevent deadlocks when buffers fill up. Output uses `WriteFile` directly, bypassing any encoding conversion performed by the C runtime.

---

## Requirements

| Software | Version |
|---|---|
| Windows | 10 or later |
| CMake | 3.20 or later |
| Visual Studio | 2019 or 2022 (MSVC) |

MinGW-w64 can also be used, but testing has been done with MSVC.

---

## Building

### With Visual Studio Generator

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

For a 32-bit build, use `-A Win32`. The output is `build\Release\cp932_test_runner.exe`.

### With Ninja (inside a Developer Command Prompt)

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output is `build\cp932_test_runner.exe`.

---

## Usage

### Via CMake FetchContent (recommended)

Add the following to your project's `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    ctest_cp932_runner
    GIT_REPOSITORY https://github.com/neige68/ctest_cp932_runner
    GIT_TAG main
)
FetchContent_MakeAvailable(ctest_cp932_runner)

enable_testing()

# Wrap a CP932-output test executable with cp932_test_runner
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:cp932_test_runner>" "$<TARGET_FILE:my_test_exe>"
)
```

### Using a Pre-built Binary

```cmake
enable_testing()

add_test(NAME my_test
    COMMAND "C:/tools/cp932_test_runner.exe" "$<TARGET_FILE:my_test_exe>"
)
```

### Passing Arguments to the Test

Arguments are forwarded to the test process as-is. Arguments containing spaces are automatically quoted.

```cmake
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:cp932_test_runner>"
        "$<TARGET_FILE:my_test_exe>"
        --input "path with spaces/data.txt"
        --verbose
)
```

---

## License

[MIT License](LICENSE)
