#pragma once
#include <iostream>

#include <cmath>
#include <algorithm>
#include <string>
#include <queue>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>

#include <faiss/IndexFlat.h>

#include "phrases.hpp"
#include "pagerank.hpp"

namespace croco {

/**
 * MultipartiteRankクラス
 *
 * @package     keyphrase
 * @author      Yujiro Takahashi <yujiro@cro-co.co.jp>
 */
class MultipartiteRank {
public:
    typedef struct _node_t {
        std::string phrase;
        float weight;
        bool operator<(const struct _node_t &node) const  {
            return node.weight > weight;
        }
    } node_t;

    typedef struct _weighted_item_t {
        std::string node_i;
        std::string node_j;
        float booster;
    } weighted_item_t;

public:
    std::vector<node_t> getKeyPhrase(const Phrases::candidate_t &candidates);

private:
    PageRank::graph_t _buildTopicGraph(const Phrases::candidate_t &candidates);
    float _getWeight(const Phrases::phrase_t *node_i, const Phrases::phrase_t *node_j);
    std::string _getFirst(const Phrases::candidate_t &candidates, std::vector<std::string> &variants);
    std::vector<std::vector<std::string>> _getVariants(const Phrases::candidate_t &candidates);
    double _getBooster(PageRank::graph_t &graph, std::string line, std::string column);
    std::map<std::pair<size_t,size_t>, double> _getWeightedEdges(const Phrases::candidate_t &candidates, const std::vector<std::vector<std::string>> topics);
    void _weightAdjustment(const Phrases::candidate_t &candidates, PageRank::graph_t &graph);

}; // class MultipartiteRank

/**
 * キーフレーズの取得
 *
 * @access public
 * @param  const Phrases::candidate_t &phrases
 * @return std::vector<MultipartiteRank::node_t>
 */
inline std::vector<MultipartiteRank::node_t> MultipartiteRank::getKeyPhrase(const Phrases::candidate_t &candidates)
{
    PageRank::graph_t graph = _buildTopicGraph(candidates);
    
    _weightAdjustment(candidates, graph);

    PageRank prank;
    auto rank = prank.execute(graph);
    std::priority_queue<node_t> queue;
    for (auto &node : rank) {
        queue.push({node.first, static_cast<float>(node.second)});
    }

    std::vector<node_t> result;
    while (!queue.empty()) {
        result.push_back(queue.top());
        queue.pop();
    }

    return result;
}

/**
 * グラフの作成
 *
 * @access private
 * @param  const Phrases::candidate_t &phrases
 * @return PageRank::graph_t
 */
inline PageRank::graph_t MultipartiteRank::_buildTopicGraph(const Phrases::candidate_t &candidates)
{
    const Phrases::phrase_t *phrases[candidates.keys.size()];
    PageRank::graph_t graph;

    size_t size = 0;
    for (auto &key : candidates.keys) {
        PageRank::node_t node;
        graph.insert(std::make_pair(key, node));
        phrases[size] = &(candidates.map.at(key));
        size++;
    }

    size_t j_start = 1;
    for (size_t i_idx=0; i_idx < size; i_idx++) {
        for (size_t j_idx=j_start; j_idx < size; j_idx++) {
            float weight = _getWeight(phrases[i_idx], phrases[j_idx]);

            std::string i_key = phrases[i_idx]->key;
            std::string j_key = phrases[j_idx]->key;
            graph.at(i_key).insert(std::make_pair(j_key, weight));
            graph.at(j_key).insert(std::make_pair(i_key, weight));
        }
        j_start++;
    }

    return graph;
}

/**
 * Weightの取得
 *
 * @access private
 * @param  Phrases::phrase_t node_i
 * @param  Phrases::phrase_t node_j
 * @return float
 */
inline float MultipartiteRank::_getWeight(const Phrases::phrase_t *node_i, const Phrases::phrase_t *node_j)
{
    std::vector<float> weights;
    for (const float &p_i : node_i->offsets) {
        for (const float &p_j : node_j->offsets) {
            float gap = std::abs(p_i - p_j);
            if (p_i < p_j) {
                gap -= node_i->words.size() - 1; 
            } else if (p_j < p_i) {
                gap -= node_j->words.size() - 1; 
            }
            weights.push_back(1.0 / gap);
        }
    }

    float result = 0.0;
    for (auto &weight: weights) {
        result = result + weight;
    }
    return result;
}

/**
 * 
 *
 * @access private
 * @param  node_t obj
 * @return std::string
 */
inline std::string MultipartiteRank::_getFirst(const Phrases::candidate_t &candidates, std::vector<std::string> &variants)
{
    std::vector<float> offsets;
    for (auto &word : variants) {
        auto &phrase = candidates.map.at(word);
        offsets.push_back(phrase.offsets.at(0));
    }
    decltype(offsets)::iterator itr = std::min_element(offsets.begin(), offsets.end());

    size_t idx = 0;
    for (auto &value : offsets) {
        if (value == *itr) {
            return variants.at(idx);
        }
        idx++;
    }
    return variants.at(idx);
}

/**
 * 
 *
 * @access private
 * @param  node_t obj
 * @return std::vector<double>
 */
inline std::vector<std::vector<std::string>> MultipartiteRank::_getVariants(const Phrases::candidate_t &candidates)
{
    std::unordered_map<std::string, size_t> wordtoidx;
    std::unordered_map<std::string, std::vector<std::string>> keytowords;
    std::vector<std::string> keys;

    size_t idx = 0;
    for (auto &key: candidates.keys) {
        for (auto &word : candidates.map.at(key).words) {
            if (wordtoidx.find(word) == wordtoidx.end()) {
                wordtoidx.insert(std::make_pair(word, idx));
                idx = idx + 1;
            } // if (map.find(word) == map.end())
        } // for (auto &word : phrase.words)
        keys.push_back(key);
        keytowords.insert(std::make_pair(key, candidates.map.at(key).words));
    } // for (auto &phrase: phrases)

    faiss::idx_t dim = wordtoidx.size();
    faiss::idx_t num = keys.size();
    std::vector<float> cluster(num * dim, 0.0);
    std::sort(keys.begin(), keys.end());

    size_t offset = 0;
    for (auto &key : keys) {
        for (auto &word : keytowords.at(key)) {
            idx = wordtoidx.at(word);
            cluster[offset + idx] += 1.0;
        }
        offset = offset + dim;
    }

    faiss::idx_t k = 4;
    float distances[num * k];
    faiss::idx_t labels[num * k];

    faiss::IndexFlatL2 index_flat(dim);
    index_flat.add(num, cluster.data());
    index_flat.search(
        num, cluster.data(), k, distances, labels
    );

    std::set<std::string> duplicate;
    for (faiss::idx_t x_idx=0; x_idx < num; x_idx++) {
        for (faiss::idx_t y_idx=0; y_idx < k; y_idx++) {
            faiss::idx_t cursor = (k *x_idx) + y_idx;
            if (1 == distances[cursor]) {
                duplicate.insert(keys.at(labels[cursor]));
            }
        }
    }

    std::vector<std::vector<std::string>> topics;
    {
        std::vector<std::string> variant;
        for (auto &word : duplicate) {
            variant.push_back(word);
        }
        std::sort(variant.begin(), variant.end());
        topics.push_back(variant);
    }

    for (auto &key : keys) {
        auto itr = duplicate.find(key);
        if (itr == duplicate.end()) {
            topics.push_back({key});
        }
    }

    return topics;
}

inline double MultipartiteRank::_getBooster(PageRank::graph_t &graph, std::string line, std::string column)
{
    auto &row = graph.at(line);
    return row.at(column);
}

/**
 * 
 *
 * @access private
 * @param  const Phrases::candidate_t &candidates
 * @param  const std::vector<std::vector<std::string>>
 * @return std::map<std::pair<size_t,size_t>, double>
 */
inline std::map<std::pair<size_t,size_t>, double> MultipartiteRank::_getWeightedEdges(const Phrases::candidate_t &candidates, const std::vector<std::vector<std::string>> topics)
{
    std::map<std::pair<size_t,size_t>, double> edges;
    size_t nv = topics.size();
    for (size_t idx_i=0; idx_i < nv; idx_i++) {
        for (size_t idx_j=idx_i + 1; idx_j < nv; idx_j++) {
            for (auto &topic_i : topics.at(idx_i)) {
                for (auto &topic_j : topics.at(idx_j)) {
                    float weight = 0.0;
                    for (auto &offset_i : candidates.map.at(topic_i).offsets) {
                        for (auto &offset_j : candidates.map.at(topic_j).offsets) {
                            float gap = std::abs(offset_i - offset_j);
                            if (offset_i < offset_j) {
                                gap -= candidates.map.at(topic_i).words.size() - 1; 
                            } else if (offset_j < offset_i) {
                                gap -= candidates.map.at(topic_j).words.size() - 1; 
                            }
                            weight += 1.0 / gap;
                        } // for (auto &offset_j : candidates.map.at(topic_j).offsets)
                    }
                    edges.insert(std::make_pair(
                        std::make_pair(idx_i, idx_j), weight
                    ));
                } // for (auto &topic_j : topics.at(idx_j))
            } // for (auto &topic_i : topics.at(idx_i))
        } // for (size_t idx_j=idx_i + 1; idx_j < number; idx_j++)
    } // for (size_t idx_i=0; idx_i < number; idx_i++)

    return edges;
}

/**
 *  Adjust edge weights for boosting some candidates.
 *  Args:
 *    alpha (float): hyper-parameter that controls the strength of the
 *         weight adjustment, defaults to 1.1.
 */
inline void MultipartiteRank::_weightAdjustment(const Phrases::candidate_t &candidates, PageRank::graph_t &graph)
{
    std::vector<weighted_item_t> weighted_edges;
    float alpha = 1.1;

    std::vector<std::vector<float>> offsets;
    auto topics = _getVariants(candidates);

    for (auto &variants : topics) {
        if (0 != variants.size()) {
            auto first = _getFirst(candidates, variants);
            for (auto &el: graph.at(first)) {
                std::vector<double> boosters;
                weighted_item_t item;
                for (auto &variant : variants) {
                    if (variant != first && el.first != variant) {
                        boosters.push_back(_getBooster(graph, variant, el.first));
                        item.node_i.assign(first);
                        item.node_j.assign(el.first);
                    }
                }
                if (boosters.size()) {
                    item.booster = std::accumulate(boosters.begin(), boosters.end(), 0.0);
                    weighted_edges.push_back(item);
                }
            } // for (auto &el: graph.at(first))
        } // if (0 != variants.size())
    }

    for (auto &item : weighted_edges) {
        auto position_i = 1.0 / (1 +candidates.map.at(item.node_i).offsets.at(0));
        position_i = std::exp(position_i);
        auto &row = graph.at(item.node_j);
        auto &weight = row.at(item.node_i);
        weight += item.booster * alpha * position_i;
    }
}

} // namespace croco