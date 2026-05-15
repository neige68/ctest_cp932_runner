# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

Windows の CTest で CP932 出力するテストプログラムをラップし、UTF-8 に変換して CTest に渡す専用テストランナー。GitHub で公開するスタンドアロンプロジェクト。

## ビルド

Visual Studio Generator（x64 Release）:

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

WSL2 から PowerShell 経由で実行する場合:

```bash
pwsh.exe -NoProfile -Command "
  Set-Location 'C:\path\to\ctest_cp932_runner'
  cmake -B build -G 'Visual Studio 17 2022' -A x64
  cmake --build build --config Release
"
```

## 構成

- `ctest_cp932_runner.cpp` — メイン実装。Windows API のみ使用（C ランタイム依存なし）
- `CMakeLists.txt` — スタンドアロンプロジェクト（C++17 / MSVC `/utf-8`）
- `README.md` — 日本語ドキュメント
- `README-EN.md` — 英語ドキュメント

## 実装の要点

- stdout / stderr を **2 スレッドで並行読み取り**（直列だとバッファ満杯でデッドロック）
- CP932→UTF-8 変換は `MultiByteToWideChar(932)` → `WideCharToMultiByte(CP_UTF8)` の 2 段階
- 出力は `WriteFile` を直接使用（C ランタイムの `printf`/`fwrite` を使うとエンコーディング変換が入る）
- 子プロセスの書き込み端ハンドルは `CreateProcess` 直後に親側で閉じる（そうしないと `ReadFile` が EOF を検出できない）
- エントリポイントは `wmain`（ワイド文字版）— 引数にパス名が含まれるため
