# c_util

C言語向けの軽量ユーティリティライブラリです。
**文字列操作、動的配列**などの低レベル機能を提供し、
将来的にはゲームエンジンや通信ライブラリの基盤として利用可能な設計を目指しています。

## 特徴

- **core_string** : 安全で柔軟な文字列操作
- **core_memory** : メモリ操作のユーティリティ(今後拡張予定)
- **message** : 軽量なログ/メッセージ出力
- **単体テスト付き**（テストコードはAI支援で生成）

今後の拡張予定：
- 動的配列モジュール（`dynamic_array`）
- ファイル操作モジュール(`filesystem`)
- CLIパーサー(`cli_parser`)
- 通信モジュール（TCP/UDP/シリアル）
- メモリ使用量のトラッキング

## ディレクトリ構成

```console
.
├── build.sh           # ビルドスクリプト
├── core
│   ├── include        # コアモジュール公開ヘッダ
│   └── src            # コアモジュール実装
│       └── internal   # コアモジュール内部管理データ
├── include            # 全モジュール共通ヘッダ
├── tests              # 単体テストコード
├── docs               # doxygenで生成されたドキュメント格納ディレクトリ
├── Doxyfile
├── LICENSE
├── makefile_test_macos.mak
└── README.md
```

## ビルド & テスト

### 必要環境

現在、下記の環境で動作確認を行なっています。
gcc、Linux環境での動作確認は今後行なっていきます。

```bash
% sw_vers
ProductName:		macOS
ProductVersion:		15.5
BuildVersion:		24F74

% /opt/homebrew/opt/llvm/bin/clang --version
Homebrew clang version 20.1.8
Target: arm64-apple-darwin24.5.0
Thread model: posix
InstalledDir: /opt/homebrew/Cellar/llvm/20.1.8/bin
Configuration file: /opt/homebrew/etc/clang/arm64-apple-darwin24.cfg
```

### ビルド

```bash
chmod +x ./build.sh
./build.sh all DEBUG_BUILD    # デバッグビルド
./build.sh all RELEASE_BUILD  # リリースビルド
./build.sh clean              # クリーン
```

### テスト実行

```bash
make -f makefile_test_macos.mak  # macOS用例
./tests/core/core_string_test
```

## ドキュメント

[Doxygenドキュメント](https://chocolate-pie24.github.io/c_util/)

### ドキュメント生成

```bash
doxygen ./Doxyfile
```

## 使用例

```c
#include "core_string.h"
#include <stdio.h>

int main(void) {
    core_string_t str = CORE_STRING_INITIALIZER;
    if (core_string_create(&str, "Hello") == CORE_STRING_OK) {
        core_string_concat(&str, ", World!");
        printf("%s\n", core_string_cstr(&str)); // 出力: Hello, World!
        core_string_destroy(&str);
    }
    return 0;
}
```

## ライセンス

このプロジェクトは **MITライセンス** で公開されています。
詳細は [LICENSE](LICENSE) を参照してください。

補足: 本プロジェクトの 単体テストおよびDoxygenコメント は AI（ChatGPT）の支援を受けて作成しています。
API実装およびコアロジックはすべて著者が作成しました。

## 作者

**chocolate-pie24**
GitHub: [https://github.com/chocolate-pie24](https://github.com/chocolate-pie24)
