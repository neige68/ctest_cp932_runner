# ctest_cp932_runner

> **Japanese:** [README.md](README.md) is also available.

A test runner wrapper for CTest on Windows that converts ANSI code page output to UTF-8. Supports CP932 (Japanese) and any other Windows ANSI code page such as CP936 (Simplified Chinese) or CP949 (Korean).

---

## When You Need This

Use this tool if all of the following apply to your project:

- You are running CMake + CTest on **Windows**
- Your test executables output text encoded in an **ANSI code page** (CP932, CP936, CP949, etc.)
- The test output contains **multibyte characters** that you want displayed correctly

ANSI code page output typically occurs in legacy software that has not yet been migrated to UTF-8.

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

`ctest_cp932_runner.exe` wraps the test process:

```
Test process (stdout/stderr: ANSI code page)
  → ctest_cp932_runner captures via pipes (two threads reading concurrently)
  → MultiByteToWideChar(code page) + WideCharToMultiByte(CP_UTF8) conversion
  → WriteFile writes UTF-8 to its own stdout/stderr
  → CTest passes the UTF-8 bytes through unchanged
  → Text is displayed correctly
```

Both stdout and stderr are read on separate threads to prevent deadlocks when buffers fill up. Output uses `WriteFile` directly, bypassing any encoding conversion performed by the C runtime.

---

## Requirements

| Software | Version |
|---|---|
| Windows | 10 or later |
| CMake | 3.20 or later |
| Visual Studio | 2022 or 2026 (MSVC) |

MinGW-w64 can also be used, but testing has been done with MSVC.

---

## Building

### With Visual Studio Generator

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

For a 32-bit build, use `-A Win32`. The output is `build\Release\ctest_cp932_runner.exe`.

### With Ninja (inside a Developer Command Prompt)

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output is `build\ctest_cp932_runner.exe`.

---

## Usage

### Specifying the Code Page

Use the `--codepage N` option to specify the code page for conversion. If omitted, the system's ANSI code page (`GetACP()`) is used automatically.

| Code Page | Language |
|---|---|
| 932 | Japanese (CP932 / Windows-31J) |
| 936 | Simplified Chinese (GBK) |
| 949 | Korean (EUC-KR compatible) |
| 950 | Traditional Chinese (Big5) |
| 1252 | Western European (Windows-1252) |

```cmake
# Explicitly specify CP932
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:ctest_cp932_runner>" --codepage 932 "$<TARGET_FILE:my_test_exe>"
)

# Use the system ANSI code page (omitted)
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:ctest_cp932_runner>" "$<TARGET_FILE:my_test_exe>"
)
```

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

add_test(NAME my_test
    COMMAND "$<TARGET_FILE:ctest_cp932_runner>" --codepage 932 "$<TARGET_FILE:my_test_exe>"
)
```

### Using a Pre-built Binary

```cmake
enable_testing()

add_test(NAME my_test
    COMMAND "C:/tools/ctest_cp932_runner.exe" --codepage 932 "$<TARGET_FILE:my_test_exe>"
)
```

### Passing Arguments to the Test

Arguments are forwarded to the test process as-is. Arguments containing spaces are automatically quoted.

```cmake
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:ctest_cp932_runner>"
        --codepage 932
        "$<TARGET_FILE:my_test_exe>"
        --input "path with spaces/data.txt"
        --verbose
)
```

---

## License

[MIT License](LICENSE)
