#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace croco {

/**
 * 配列の要素数と次元から行数を検証・決定する
 *
 * かつては number = size / d の切り捨てで決めていたため、要素数が次元の倍数で
 * ない配列は 0 行として黙って捨てられていた（faiss の ID と呼び出し側の添字が
 * ずれる原因）。端数が出る入力・行数と合わない number 指定は例外にする。
 * zend / faiss 非依存（tests/test_croco_validate.cc のホスト単体テスト対象）
 */
inline int64_t resolveRowCount(size_t size, int64_t number, int64_t dimension)
{
    if (dimension <= 0) {
        // new \Croco\Faiss(0) は index_factory を通ってしまうため、ここで弾かないと
        // size % 0 の 0 除算で PHP プロセスごと落ちる
        throw std::invalid_argument("index dimension must be positive");
    }
    if (0 == size || 0 != (size % static_cast<size_t>(dimension))) {
        throw std::invalid_argument(
            "array length (" + std::to_string(size)
            + ") must be a positive multiple of dimension (" + std::to_string(dimension) + ")");
    }
    // 乗算 (number * dimension) は size_t で wrap しうるため、検証は除算結果との比較で行う
    const int64_t rows = static_cast<int64_t>(size / static_cast<size_t>(dimension));
    if (0 == number) {
        return rows;
    }
    if (number != rows) {
        throw std::invalid_argument(
            "number (" + std::to_string(number) + ") does not match array length ("
            + std::to_string(size) + ") / dimension (" + std::to_string(dimension) + ")");
    }
    return number;
}

} // namespace croco
