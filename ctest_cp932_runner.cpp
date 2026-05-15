// ctest_cp932_runner.cpp
//
// Copyright (c) 2026 neige68
// https://github.com/neige68/ctest_cp932_runner
// SPDX-License-Identifier: MIT
//
// ANSI コードページ出力を UTF-8 に変換して ctest に渡すテストランナー
// Usage: ctest_cp932_runner.exe [--codepage N] <test_exe> [args...]
//
// test_exe の stdout/stderr を指定コードページ（デフォルト: GetACP()）として
// キャプチャし、UTF-8 に変換して自身の stdout/stderr へ WriteFile で出力する。
// test_exe の終了コードをそのまま返す。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <thread>

// バイト列を指定コードページから UTF-8 に変換する
static std::string to_utf8(const std::vector<BYTE>& bytes, UINT codepage)
{
    if (bytes.empty())
        return {};

    // codepage -> UTF-16
    int wlen = MultiByteToWideChar(codepage, 0,
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<int>(bytes.size()),
        nullptr, 0);
    if (wlen <= 0)
        return {};
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(codepage, 0,
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<int>(bytes.size()),
        wstr.data(), wlen);

    // UTF-16 -> UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0,
        wstr.data(), wlen,
        nullptr, 0, nullptr, nullptr);
    if (u8len <= 0)
        return {};
    std::string u8str(u8len, '\0');
    WideCharToMultiByte(CP_UTF8, 0,
        wstr.data(), wlen,
        u8str.data(), u8len, nullptr, nullptr);

    return u8str;
}

// パイプから全バイトを読み取ってハンドルを閉じる
static std::vector<BYTE> read_all(HANDLE h)
{
    std::vector<BYTE> buf;
    BYTE tmp[4096];
    DWORD n;
    while (ReadFile(h, tmp, sizeof(tmp), &n, nullptr) && n > 0)
        buf.insert(buf.end(), tmp, tmp + n);
    CloseHandle(h);
    return buf;
}

// argv[start..] をスペース区切りのコマンドライン文字列に結合（引数をクォート）
static std::wstring build_command_line(int argc, wchar_t* argv[], int start)
{
    std::wstring cmd;
    for (int i = start; i < argc; ++i) {
        if (i > start)
            cmd += L' ';
        std::wstring arg = argv[i];
        bool need_quote = arg.empty()
            || arg.find_first_of(L" \t\"") != std::wstring::npos;
        if (need_quote) {
            cmd += L'"';
            for (wchar_t c : arg) {
                if (c == L'"')
                    cmd += L'\\';
                cmd += c;
            }
            cmd += L'"';
        } else {
            cmd += arg;
        }
    }
    return cmd;
}

static void write_all(HANDLE h, const std::string& s)
{
    if (!s.empty()) {
        DWORD written;
        WriteFile(h, s.data(), static_cast<DWORD>(s.size()), &written, nullptr);
    }
}

int wmain(int argc, wchar_t* argv[])
{
    UINT codepage = GetACP();
    int exe_arg = 1;

    if (argc >= 3 && wcscmp(argv[1], L"--codepage") == 0) {
        codepage = static_cast<UINT>(_wtoi(argv[2]));
        if (codepage == 0) {
            const char msg[] = "ctest_cp932_runner: --codepage: invalid value\r\n";
            DWORD written;
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg) - 1, &written, nullptr);
            return 1;
        }
        exe_arg = 3;
    }

    if (argc <= exe_arg) {
        const char msg[] = "Usage: ctest_cp932_runner.exe [--codepage N] <test_exe> [args...]\r\n";
        DWORD written;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg) - 1, &written, nullptr);
        return 1;
    }

    // パイプ作成（子の stdout / stderr をそれぞれキャプチャ）
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hOutRead = nullptr, hOutWrite = nullptr;
    HANDLE hErrRead = nullptr, hErrWrite = nullptr;
    if (!CreatePipe(&hOutRead, &hOutWrite, &sa, 0))
        return 1;
    if (!CreatePipe(&hErrRead, &hErrWrite, &sa, 0)) {
        CloseHandle(hOutRead);
        CloseHandle(hOutWrite);
        return 1;
    }
    SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0);

    // 子プロセス起動
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hOutWrite;
    si.hStdError  = hErrWrite;

    std::wstring cmd = build_command_line(argc, argv, exe_arg);
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(nullptr, cmd.data(),
        nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    // 子が使う書き込み端を親側で閉じる（ReadFile が EOF を検出できるように）
    CloseHandle(hOutWrite);
    CloseHandle(hErrWrite);

    if (!ok) {
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
        return 1;
    }

    // stdout / stderr を並行して読み取る（直列だとデッドロックの可能性がある）
    std::vector<BYTE> out_bytes, err_bytes;
    std::thread out_thread([&]() { out_bytes = read_all(hOutRead); });
    std::thread err_thread([&]() { err_bytes = read_all(hErrRead); });
    out_thread.join();
    err_thread.join();

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 指定コードページ -> UTF-8 変換して WriteFile 出力
    // （C ランタイムの stdout/stderr は使わない。エンコーディング変換を回避するため）
    write_all(GetStdHandle(STD_OUTPUT_HANDLE), to_utf8(out_bytes, codepage));
    write_all(GetStdHandle(STD_ERROR_HANDLE),  to_utf8(err_bytes, codepage));

    return static_cast<int>(exit_code);
}
