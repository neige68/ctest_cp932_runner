# ctest_cp932_runner

> **English:** [README-EN.md](README-EN.md) is also available.

Windows の CTest で CP932 出力するテストプログラムを正しく扱うためのテストランナーラッパーです。

---

## このツールが必要な場合

次の条件に当てはまるプロジェクトで使用します。

- **Windows** 上で CMake + CTest を使っている
- テストプログラムが **CP932（Windows-31J / Shift_JIS）** でテキストを出力する
- テスト出力に **日本語が含まれており、文字化けなく表示したい**

CP932 出力が残っているのは、ほとんどの場合、UTF-8 移行が完了していないレガシーな日本語ソフトウェアです。

---

## なぜ必要か

### CTest の UTF-8 処理

CTest（CMake 3.20 以降）はテストプロセスの出力を **UTF-8 バイト列として処理** します。CP932 のマルチバイト文字のリードバイト（0x80–0xFF）は UTF-8 として無効なため、CTest はこれらを **U+FFFD（`\xEF\xBF\xBD`）に置換** します。結果として、テスト出力に含まれる日本語がすべて `?` に化けます。

```
期待: [ERROR] ファイルが見つかりません: config.txt
実際: [ERROR] ??????????????????????: config.txt
```

### `cmd.exe /C` を `add_test` に使えない理由

`add_test(COMMAND cmd.exe /C ...)` は使えません。CTest は `COMMAND` の各引数を **個別にクォート** して `CreateProcess` に渡すため、`/C` が `"/C"` になり、cmd.exe がフラグとして認識できず次のエラーで失敗します。

```
コマンドの構文が誤っています。
```

### このツールの解決策

`cp932_test_runner.exe` がテストプロセスをラップします。

```
テストプロセス (stdout/stderr: CP932)
  → cp932_test_runner がパイプでキャプチャ（2 スレッドで並行読み取り）
  → MultiByteToWideChar(CP 932) + WideCharToMultiByte(CP_UTF8) で変換
  → WriteFile で UTF-8 を自身の stdout/stderr へ出力
  → CTest が UTF-8 バイト列をそのまま通過させる
  → 文字化けなしに正しい日本語が表示される
```

stdout と stderr を 2 スレッドで並行読み取りすることで、バッファが満杯になったときのデッドロックを防ぎます。出力には `WriteFile` を使用し、C ランタイムによるエンコーディング変換を回避します。

---

## ビルドに必要なソフトウェア

| ソフトウェア | バージョン |
|---|---|
| Windows | 10 以降 |
| CMake | 3.20 以上 |
| Visual Studio | 2019 または 2022（MSVC コンパイラ）|

MinGW-w64 でもビルド可能ですが、動作確認は MSVC で行っています。

---

## ビルド方法

### Visual Studio Generator を使う場合

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

32 ビット版が必要な場合は `-A Win32` に変更します。ビルド後に `build\Release\cp932_test_runner.exe` が生成されます。

### Ninja を使う場合（Developer Command Prompt 内で）

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

ビルド後に `build\cp932_test_runner.exe` が生成されます。

---

## 使用方法

### CMake FetchContent で組み込む（推奨）

自分のプロジェクトの `CMakeLists.txt` に以下を追加します。

```cmake
include(FetchContent)
FetchContent_Declare(
    ctest_cp932_runner
    GIT_REPOSITORY https://github.com/neige68/ctest_cp932_runner
    GIT_TAG main
)
FetchContent_MakeAvailable(ctest_cp932_runner)

enable_testing()

# CP932 出力するテストを cp932_test_runner でラップする
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:cp932_test_runner>" "$<TARGET_FILE:my_test_exe>"
)
```

### ビルド済みバイナリを直接参照する場合

```cmake
enable_testing()

add_test(NAME my_test
    COMMAND "C:/tools/cp932_test_runner.exe" "$<TARGET_FILE:my_test_exe>"
)
```

### 引数付きのテストを実行する

テストプロセスへの引数はそのまま渡されます。スペースを含む引数は自動的にクォートされます。

```cmake
add_test(NAME my_test
    COMMAND "$<TARGET_FILE:cp932_test_runner>"
        "$<TARGET_FILE:my_test_exe>"
        --input "path with spaces/data.txt"
        --verbose
)
```

---

## ライセンス

[MIT License](LICENSE)
