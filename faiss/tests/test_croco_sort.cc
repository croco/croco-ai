// croco_sort.hpp のホスト単体テスト（zend / faiss 非依存）
//
// ビルド・実行:
//   g++ -std=c++17 -Wall -Wextra -I../classes -o test_croco_sort test_croco_sort.cc && ./test_croco_sort
#include <cassert>
#include <cstdio>
#include "croco_sort.hpp"

int main() {
    // 1) N 件がすべて返る（旧実装は pop() で size() が縮み ceil(N/2) 件で打ち切り）
    {
        float d[5] = {5.f, 4.f, 3.f, 2.f, 1.f};
        int64_t l[5] = {10, 20, 30, 40, 50};
        auto r = croco::FaissStatsFormat(d, l, 5);
        assert(r.size() == 5);
        // 距離昇順（priority_queue の top が最小距離）
        assert(r[0].id == 50 && r[4].id == 10);
        for (size_t i = 1; i < r.size(); i++) assert(r[i-1].distance <= r[i].distance);
    }
    // 2) 同一ラベルの集約（count と平均距離）
    {
        float d[4] = {1.f, 3.f, 10.f, 20.f};
        int64_t l[4] = {7, 7, 8, 8};
        auto r = croco::FaissStatsFormat(d, l, 4);
        assert(r.size() == 2);
        assert(r[0].id == 7 && r[0].count == 2 && r[0].distance == 2.0f);
        assert(r[1].id == 8 && r[1].count == 2 && r[1].distance == 15.0f);
    }
    // 3) 番兵 (-1) はスキップ（k 件に満たないとき faiss が埋める）
    {
        float d[3] = {1.f, 3.4e38f, 3.4e38f};
        int64_t l[3] = {5, -1, -1};
        auto r = croco::FaissStatsFormat(d, l, 3);
        assert(r.size() == 1 && r[0].id == 5);
    }
    // 4) size 0 は空
    {
        auto r = croco::FaissStatsFormat(nullptr, nullptr, 0);
        assert(r.empty());
    }
    // 5) 64bit ラベルが壊れない（旧実装は int に切り捨て）
    {
        float d[1] = {1.f};
        int64_t l[1] = {int64_t(1) << 40};
        auto r = croco::FaissStatsFormat(d, l, 1);
        assert(r[0].id == (int64_t(1) << 40));
    }
    // 6) 負の ID (-1 以外) は落とさない
    {
        float d[2] = {1.f, 2.f};
        int64_t l[2] = {-5, -1};
        auto r = croco::FaissStatsFormat(d, l, 2);
        assert(r.size() == 1 && r[0].id == -5);
    }
    puts("all tests passed");
    return 0;
}
