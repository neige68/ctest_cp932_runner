// helper_cp932_output.cpp
//
// CP932 バイト列を stdout/stderr に出力するテストヘルパー
// ctest_cp932_runner のテスト用。ソースは UTF-8 で書き、
// WideCharToMultiByte で CP932 に変換して WriteFile で出力する。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

static std::string to_cp932(const wchar_t* s)
{
    int len = WideCharToMultiByte(932, 0, s, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return {};
    std::string buf(len, '\0');
    WideCharToMultiByte(932, 0, s, -1, buf.data(), len, nullptr, nullptr);
    buf.resize(len - 1);  // null terminator を除く
    return buf;
}

static void write_cp932(HANDLE h, const wchar_t* s)
{
    std::string cp932 = to_cp932(s);
    DWORD written;
    WriteFile(h, cp932.data(), static_cast<DWORD>(cp932.size()), &written, nullptr);
}

int main()
{
    write_cp932(GetStdHandle(STD_OUTPUT_HANDLE), L"stdout: 日本語テスト出力\n");
    write_cp932(GetStdHandle(STD_ERROR_HANDLE),  L"stderr: 標準エラー出力\n");
    return 0;
}
