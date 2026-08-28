// croco_validate.hpp（resolveRowCount）のホスト単体テスト（zend / faiss 非依存）
//
// ビルド・実行:
//   g++ -std=c++17 -Wall -Wextra -I../classes -o test_croco_validate test_croco_validate.cc && ./test_croco_validate
#include <cassert>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include "croco_validate.hpp"

static bool throws_invalid(const std::function<void()> &fn)
{
    try {
        fn();
    } catch (const std::invalid_argument &) {
        return true;
    }
    return false;
}

int main() {
    // 正常系
    assert(croco::resolveRowCount(8, 0, 4) == 2);   // number 省略 → size / d
    assert(croco::resolveRowCount(8, 2, 4) == 2);   // number 明示・一致
    assert(croco::resolveRowCount(4, 1, 4) == 1);   // 1 行ちょうど

    // 異常系
    assert(throws_invalid([]{ croco::resolveRowCount(0, 0, 4); }));   // 空配列（silent drop 防止）
    assert(throws_invalid([]{ croco::resolveRowCount(6, 0, 4); }));   // 端数（切り捨て防止）
    assert(throws_invalid([]{ croco::resolveRowCount(8, 3, 4); }));   // number 不一致
    assert(throws_invalid([]{ croco::resolveRowCount(8, -2, 4); }));  // 負の number
    assert(throws_invalid([]{ croco::resolveRowCount(8, 0, 0); }));   // 次元 0（size % 0 の SIGFPE 防止）
    assert(throws_invalid([]{ croco::resolveRowCount(8, 0, -4); })); // 負の次元

    // number * dimension の size_t wrap による検証迂回の回帰
    // (2^62 + 1) * 4 は 2^64 + 4 に wrap して 4 == size を偽装できた
    assert(throws_invalid([]{ croco::resolveRowCount(4, (int64_t(1) << 62) + 1, 4); }));

    puts("all tests passed");
    return 0;
}
