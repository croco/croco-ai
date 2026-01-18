#pragma once

#include <queue>
#include <unordered_map>
#include <vector>

namespace croco {

typedef struct _stats {
    int id;
    int count;
    float distance;
    bool operator<(const struct _stats &stats) const  {
        return distance > stats.distance;
    }
} stats_t;

std::vector<stats_t> FaissStatsFormat(float *distances, long *labels, size_t size);

/**
 * get stats format
 *
 * @access public
 * @return Stats*
 */
std::vector<stats_t> FaissStatsFormat(float *distances, long *labels, size_t size)
{
    std::unordered_map<long, std::vector<size_t>> sumidx;
    for (size_t idx=0; idx < size; idx++) {
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

    std::vector<stats_t> result;
    for (size_t idx=0; idx < queue.size(); idx++) {
        result.push_back({
            id:queue.top().id,
            count:queue.top().count,
            distance:queue.top().distance
        });
        queue.pop();
    }

    return result;
}

} // namespace croco