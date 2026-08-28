#pragma once

#include <cmath>

#include <algorithm>
#include <numeric>
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace croco {

/**
 * Page Rank Class
 *
 * @package     pageRank
 * @author      Yujiro Takahashi <yujiro@cro-co.co.jp>
 */
class PageRank {
public:
    typedef std::unordered_map<std::string, double> node_t;
    typedef std::unordered_map<std::string, node_t> graph_t;

public:
    node_t execute(graph_t G);

private:
    std::vector<double> _values(node_t obj);
    double _sum(std::vector<double> xs);
}; // class PageRank

/**
 * グラフからページランクを算出
 *
 * G は候補数 n に対して n^2 の辺を持つ完全グラフなので、コピーが 1 本増えるだけで
 * ピークメモリが 1/3 ずつ増える。呼び出し側から std::move で受け取り、正規化も
 * 別の graph_t を作らず G の中で行うことで、この関数が抱える n^2 構造を 1 本に
 * 抑える（croco-ai#7 のレビュー指摘。本番の解析対象に 89,315 文字のページが実在し、
 * そこでピーク 3.3GB / Lambda 1024MB が OOM していた）
 *
 * @access private
 * @param  graph_t G  値渡し。呼び出し側は std::move で渡すこと
 * @return node_t
 */
inline PageRank::node_t PageRank::execute(graph_t G)
{
    node_t personalization, dangling;
    int max_iter = 100;
    double alpha = 0.74;
    double tol = 1.0e-6;

    std::size_t N = G.size();
    if (0 == N) {
        throw std::logic_error("Graph data required.");
    }

    /* 各行を出次数で割って遷移確率にする（G を書き換えるので追加の確保は無い） */
    for (auto &line : G) {
        double node_degree = 0.0f;
        for (auto &node : line.second) {
            node_degree += node.second;
        }

        for (auto &node : line.second) {
            node.second = node.second / node_degree;
        }
    }
    /*
     * 旧実装は別の graph_t へ insert し直していたので unordered_map の反復順が
     * G と違った。べき乗法の内側の加算順が変わるぶん結果は最終桁で動きうるが、
     * 収束判定が err < N * 1.0e-6 なのでその範囲。実測（croco-ai#7 の記事）では
     * 重みが小数第 6 位まで一致した
     */
    graph_t &W = G;

    node_t x;
    for (auto &node : W) {
        x.insert(std::make_pair(node.first, (1.0 / N)));
    }

    node_t p;
    if (!personalization.size()) {
        for (auto &node : W) {
            p.insert(std::make_pair(node.first, (1.0 / N)));
        }
    } else {
        double sum = _sum(_values(personalization));
        for (auto &node : personalization) {
            p.insert(std::make_pair(node.first, (node.second / sum)));
        }
    } // if (!personalization.size())

    node_t dangling_weights;
    if (!dangling_weights.size()) {
        dangling_weights = p;
    } else {
        double sum = _sum(_values(dangling));
        for (auto &node : dangling) {
            p.insert(std::make_pair(node.first, (node.second / sum)));
        }
    } // if (!dangling_weights.size())

    std::vector<std::string> dangling_nodes;
    for (auto &line : W) {
        if (line.second.size() == 0) {
            dangling_nodes.push_back(line.first);
        }
    } // for (auto &line : W)


    // power iteration: make up to max_iter iterations
    for (int iter_count = 0; iter_count < max_iter; iter_count++) {
        node_t xlast = x;
        x.clear();

        for (auto &node : xlast) {
            x[node.first] = 0.0;
        }
        double sum = 0.0;

        for (auto &node : dangling_nodes) {
            sum += xlast.at(node);
        }

        double danglesum = alpha * sum;
        for (auto &node : x) {
            for (auto &nbr : W.at(node.first)) {
                x[nbr.first] += alpha * xlast.at(node.first) * W.at(node.first).at(nbr.first);
            }
            x[node.first] += danglesum * dangling_weights.at(node.first) + (1.0 - alpha) * p.at(node.first);
        }

        double err = 0.0f;
        for (auto &node : x) {
            err += std::abs(node.second - xlast.at(node.first));
        }
        if (err < N * tol) {
            return x;
        }
    }

    return x;
}

/**
 * 数値部分の抽出
 *
 * @access private
 * @param  node_t obj
 * @return std::vector<double>
 */
inline std::vector<double> PageRank::_values(node_t obj)
{
    std::vector<double> xs;
    for (auto &val : obj) {
        xs.push_back(val.second);
    }
    return xs;
}

/**
 * 配列の合計
 *
 * @access private
 * @param  std::vector<double> xs
 * @return double
 */
inline double PageRank::_sum(std::vector<double> xs)
{
    return std::accumulate(xs.begin(), xs.end(), 0);
}

} // namespace croco