#pragma once

#include <string>
#include <vector>

namespace croco {

/**
 * 行解析クラス
 *
 * @package     keyphrase
 * @author      Yujiro Takahashi <yujiro@cro-co.co.jp>
 */
class Lines {
public:
    typedef struct _line_t {
        std::vector<std::string> words;
        std::vector<unsigned short> pos;
        float shift;
        void line_t() {
            clear();
        }
        void clear() {
            words.clear();
            pos.clear();
            shift = 0;
        }
    } line_t;
    typedef struct _sentence_t {
        std::vector<std::string> words;
        std::vector<unsigned short> pos;
    } sentence_t;

public:
    std::vector<line_t> parse(std::vector<std::vector<std::string>> &wordLines, std::vector<std::vector<unsigned short>> &posLines);

private:
    float _getShift(std::vector<line_t> &nodes, size_t range);

}; // class Lines

/**
 * 文節毎のshift値計算
 *
 * @access public
 * @param  const std::string text
 * @return std::vector<Lines::line_t>
 */
inline std::vector<Lines::line_t> Lines::parse(std::vector<std::vector<std::string>> &wordLines, std::vector<std::vector<unsigned short>> &posLines)
{
    std::vector<line_t> result;

    for (size_t lidx=0; lidx < wordLines.size(); lidx++) {
        line_t line;
        for (size_t ridx=0; ridx < wordLines.at(lidx).size(); ridx++) {
            line.words.push_back(wordLines.at(lidx).at(ridx));
            line.pos.push_back(posLines.at(lidx).at(ridx));
        }
        result.push_back(line);
    }

    for (size_t idx=0; idx < result.size(); idx++) {
        result.at(idx).shift = _getShift(result, idx);
    }

    return result;
}

/**
 * Shift値の取得　（単語構成サイズ）
 *
 * @access private
 * @param  std::vector<line_t> &nodes
 * @param  size_t range
 * @return float
 */
inline float Lines::_getShift(std::vector<line_t> &nodes, size_t range)
{
    float shift = 0.0;
     
    for (size_t idx = 0; idx < range; idx++) {
        shift = shift + nodes.at(idx).words.size();
    }
    return shift;
}

} // namespace croco