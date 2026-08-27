# faiss 拡張のテスト

## ホスト単体テスト（zend / faiss 非依存）

`croco_sort.hpp`（結果集約）と `croco_validate.hpp`（入力検証）は純 C++ のため、
拡張をビルドしなくても手元の g++ だけで回せる。

```sh
cd tests
g++ -std=c++17 -Wall -Wextra -I../classes -o test_croco_sort test_croco_sort.cc && ./test_croco_sort
g++ -std=c++17 -Wall -Wextra -I../classes -o test_croco_validate test_croco_validate.cc && ./test_croco_validate
```

cmake がある環境なら ctest 経由でも実行できる:

```sh
cmake -DBUILD_TESTING=ON . && make test_croco_sort test_croco_validate && ctest
```

## PHP 受け入れテスト（実機ビルド環境用）

拡張とfaiss をビルドした環境（Alpine コンテナ等）で実行する。zend 境界のコード
（型検査・例外化・バッファ確保・並び順・arginfo）はここでしか検証できない。

```sh
php -d extension=/path/to/croco_faiss.so tests/test_faiss.php             # add / addWithIds / search（issue #1 / PR #2）
php -d extension=/path/to/croco_faiss.so tests/test_faiss_reconstruct.php # reconstruct（issue #3 / PR #5）
php -d extension=/path/to/croco_faiss.so tests/test_faiss_metric.php      # metric 別並び順・arginfo（issue #4 / PR #6）
```

- 失敗があると exit 1、全部通ると exit 0
- 拡張がロードされていないと exit 77（SKIP）で、誤って green と数えない
