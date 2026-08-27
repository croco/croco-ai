#pragma once

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

namespace croco {

typedef struct _stats {
    int64_t id;
    int count;
    float distance;
    bool operator<(const struct _stats &stats) const  {
        return distance > stats.distance;
    }
} stats_t;

std::vector<stats_t> FaissStatsFormat(const float *distances, const int64_t *labels, size_t size);

/**
 * get stats format
 *
 * @access public
 * @return Stats*
 */
std::vector<stats_t> FaissStatsFormat(const float *distances, const int64_t *labels, size_t size)
{
    std::unordered_map<int64_t, std::vector<size_t>> sumidx;
    for (size_t idx=0; idx < size; idx++) {
        if (labels[idx] < 0) {
            continue; // k > ntotal のとき faiss が埋める番兵 (-1) は結果に含めない
        }
        if (sumidx.find(labels[idx]) == sumidx.end()) {
            std::vector<size_t> idxs = { idx };
            sumidx.insert(std::make_pair(labels[idx], idxs));
        } else {
            sumidx.at(labels[idx]).push_back(idx);
        }
    }

    std::priority_queue<struct _stats> queue;
    for (auto row : sumidx) {
        struct _stats val;
        val.id = row.first;
        val.count = row.second.size();

        float distance = 0.0f;
        for (auto idx : row.second) {
            distance += distances[idx];
        }
        val.distance = distance / val.count;
        queue.push(val);
    }

    // pop() のたびに size() が縮むため for (idx < queue.size()) では半分しか取り出せない。
    // 空になるまで回す
    std::vector<stats_t> result;
    while (!queue.empty()) {
        result.push_back(queue.top());
        queue.pop();
    }

    return result;
}

} // namespace croco
