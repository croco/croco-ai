#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace croco {

typedef struct _stats {
    int64_t id;
    int count;
    float distance;
} stats_t;

inline std::vector<stats_t> FaissStatsFormat(const float *distances, const int64_t *labels, size_t size, bool ascending = true);

/**
 * get stats format
 *
 * @access public
 * @return Stats*
 */
inline std::vector<stats_t> FaissStatsFormat(const float *distances, const int64_t *labels, size_t size, bool ascending)
{
    std::unordered_map<int64_t, std::vector<size_t>> sumidx;
    for (size_t idx=0; idx < size; idx++) {
        if (labels[idx] == -1) {
            // 候補が k 件に満たないとき faiss が埋める番兵は -1 固定。
            // < 0 で判定すると addWithIds() で負の ID を登録したベクトルまで落ちる。
            // 制約: 他ツール（Python 版 faiss 等）が ID -1 で登録したインデックスを
            // readIndex()/importIndex() で読み込んだ場合、そのベクトルは番兵と
            // 区別できずここで落ちる（登録側では addWithIds() が -1 を拒否する）
            continue;
        }
        if (sumidx.find(labels[idx]) == sumidx.end()) {
            std::vector<size_t> idxs = { idx };
            sumidx.insert(std::make_pair(labels[idx], idxs));
        } else {
            sumidx.at(labels[idx]).push_back(idx);
        }
    }

    std::vector<stats_t> result;
    result.reserve(sumidx.size());
    for (const auto &row : sumidx) {
        struct _stats val;
        val.id = row.first;
        val.count = row.second.size();

        float distance = 0.0f;
        for (auto idx : row.second) {
            distance += distances[idx];
        }
        val.distance = distance / val.count;
        result.push_back(val);
    }

    // L2 等の距離系メトリックは値が小さいほど良いので昇順、
    // 内積等の類似度系メトリックは値が大きいほど良いので降順に並べる
    std::sort(result.begin(), result.end(), [ascending](const stats_t &a, const stats_t &b) {
        if (a.distance != b.distance) {
            return ascending ? a.distance < b.distance : a.distance > b.distance;
        }
        return a.id < b.id; // 同値距離の順序を決定的にする（unordered_map の走査順に依存させない）
    });

    return result;
}

} // namespace croco
