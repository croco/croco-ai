# keyphrase 拡張のテスト

## ホスト単体テスト（zend / mecab / faiss 非依存）

`phrases.hpp`（候補フレーズの切り出しとフィルタ）は純 C++ のため、拡張をビルド
しなくても手元の g++ だけで回せる。分かち書き済みの語と posid を直接与える。

```sh
cd keyphrase/tests
g++ -std=c++17 -Wall -Wextra -I../include -o test_phrases test_phrases.cc && ./test_phrases
```

cmake がある環境なら ctest 経由でも実行できる（`CMakeLists.txt` は `keyphrase/` に
あるので、そちらで実行する）:

```sh
cd keyphrase
cmake -DBUILD_TESTING=ON . && make test_phrases && ctest
```

どちらもイン・ソースビルドで `keyphrase/` に中間生成物を撒く。`.gitignore` に入れて
あるので git には出ないが、消すなら `CMakeCache.txt` / `CMakeFiles/` / `Makefile` /
`cmake_install.cmake` / `CTestTestfile.cmake` / `Testing/` / `tests/test_phrases`。

## PHP 受け入れテスト（実機ビルド環境用）

拡張と mecab をビルドした環境（Alpine コンテナ等）で実行する。実際の分かち書き・
posid・未知語推定・行の切り方を通した結果は、ここでしか検証できない。

```sh
php -d extension=/path/to/croco_keyphrase.so tests/test_keyphrase.php   # 候補の切り出し（issue #7 / PR #8）
```

- 失敗があると exit 1、全部通ると exit 0
- 拡張がロードされていないと exit 77（SKIP）で、誤って green と数えない
- 辞書は `/usr/lib/mecab/dic/ipadic` を見る。別の場所なら `MECAB_DIC_PATH` で渡す
