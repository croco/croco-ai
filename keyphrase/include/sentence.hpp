#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <mecab.h>

namespace croco {

/**
 * 行解析クラス
 *
 * @package     keyphrase
 * @author      Yujiro Takahashi <yujiro@cro-co.co.jp>
 */
class Sentence {
public:
    typedef struct _sentences {
        std::vector<std::vector<std::string>> wordLines;
        std::vector<std::vector<unsigned short>> posLines;
    } Sentences;
private:
    MeCab::Tagger *_tagger;

public:
    Sentence(std::string dictionary) { 
        std::string option("-d" + dictionary); 
        _tagger = MeCab::createTagger(option.c_str());
    }
    ~Sentence() {
        if (nullptr != _tagger) {
            delete _tagger;
        }
    }
    Sentences parse(std::string text);
    std::vector<std::string> explode(const std::string str, const char delimiter);
    std::vector<std::vector<std::string>> explodeSentence(const std::string str, bool isWakati=false);
private:
    bool _isPeriod(const MeCab::Node* node);
    int _utf8Strlen(const std::string word);
}; // class Sentence

/**
 * 辞書の読み込み
 *
 * @access public
 * @param  std::string text
 * @return Sentence::Sentences
 */
inline Sentence::Sentences Sentence::parse(std::string text)
{
    Sentences result;
    std::vector<std::string> lines =  explode(text, '\n');
    std::vector<std::string> wordRow;
    std::vector<unsigned short> posRow;

    for (auto &line : lines) {
        const MeCab::Node* node = _tagger->parseToNode(line.c_str());
        for (; node; node = node->next) {
            if (node->stat != MECAB_BOS_NODE && node->stat != MECAB_EOS_NODE) {
                std::string surface(node->surface, 0, node->length);
                wordRow.push_back(surface);
                posRow.push_back(node->posid);
                if (_isPeriod(node)) {
                    result.wordLines.push_back(wordRow);
                    result.posLines.push_back(posRow);
                    wordRow.clear();
                    posRow.clear();
                }
            } // if (node->stat != MECAB_BOS_NODE && node->stat != MECAB_EOS_NODE)
        } // for (; node; node = node->next)

        // 行末でも切る。'\n' で分割して行ごとに tagger へ渡している以上、行はすでに
        // 解析の境界になっている。ここで切らないと wordRow が行をまたいで積み上がり、
        // 句点で終わらない行（見出しなど）が次の行の本文と 1 センテンスに繋がる。
        // Phrases が連続する名詞列をフレーズにするため、見出し末尾と本文先頭が
        // くっついた `勉強コスト独学` のような候補が生まれていた (#7)
        if (wordRow.size()) {
            result.wordLines.push_back(wordRow);
            result.posLines.push_back(posRow);
            wordRow.clear();
            posRow.clear();
        }
    }

    return result;
}

/**
 * 指定文字による分割
 *
 * @access public
 * @param  const std::string str
 * @param  const char delimiter
 * @return std::vector<std::string>
 */
inline std::vector<std::string> Sentence::explode(const std::string str, const char delimiter)
{
    std::vector<std::string> result;

    size_t pos = str.find(delimiter);
    size_t last = 0;

    while (pos != std::string::npos) {
        size_t size = pos - last;
        result.push_back(str.substr(last, size));

        last = pos + 1;
        pos = str.find(delimiter, pos + 1);
    } // while (pos != std::string::npos)

    if (str.length() > last) {
        result.push_back(str.substr(last));
    }

    return result;
}

/**
 * センテンス毎の分割
 *
 * @access public
 * @param  const std::string str
 * @return std::vector<std::string>
 */
inline std::vector<std::vector<std::string>> Sentence::explodeSentence(const std::string str, bool isWakati)
{
    std::vector<std::vector<std::string>> result;

    std::vector<std::string> lines =  explode(str, '\n');
    std::vector<std::string> sentence;
    std::string wakati = isWakati ? " ": "";

    for (auto &line : lines) {
        const MeCab::Node* node = _tagger->parseToNode(line.c_str());
        for (; node; node = node->next) {
            if (node->stat != MECAB_BOS_NODE && node->stat != MECAB_EOS_NODE) {
                std::string surface(node->surface, 0, node->length);

                sentence.push_back(surface);
                if (_isPeriod(node)) {
                    result.push_back(sentence);
                    sentence.clear();
                }
            } // if (node->stat != MECAB_BOS_NODE && node->stat != MECAB_EOS_NODE)
        } // for (; node; node = node->next)

        // parse() と同じ理由で行末でも切る（#7）
        if (sentence.size()) {
            result.push_back(sentence);
            sentence.clear();
        }
    }

    return result;
}

/**
 * 句点などの文節判定
 *
 * @access private
 * @param  const std::string str
 * @return std::vector<std::string>
 */
inline bool Sentence::_isPeriod(const MeCab::Node* node)
{
    bool result = false;
    if (7 == node->posid) {
        result = true;
    } else if (4 == node->posid) {
        std::string surface(node->surface, 0, node->length);
        if ("！" == surface || "!" == surface) {
            result = true;
            const MeCab::Node* _next = node->next;
            if (4 == _next->posid) {
                std::string _surface(_next->surface, 0, _next->length);
                if ("？" == _surface || "?" == _surface) {
                    result = false;
                }
            }
        } else if ("？" == surface || "?" == surface) {
            result = true;
            const MeCab::Node* _next = node->next;
            if (4 == _next->posid) {
                std::string _surface(_next->surface, 0, _next->length);
                if ("！" == _surface || "!" == _surface) {
                    result = false;
                }
            }
        }
    }
    return result;
}

/**
 * utf8文字数カウンタ
 *
 * @access private
 * @param  const std::string pattern
 * @return int
 */
inline int Sentence::_utf8Strlen(const std::string word)
{
    size_t length = 0;
    for (size_t pos = 0; pos < word.size();) {
        uint8_t hex = static_cast<uint8_t>(word[pos]);
        pos += (hex < 0x80) ? 1 :
               (hex < 0xE0) ? 2 :
               (hex < 0xF0) ? 3 : 4;
        length += 1;
    }

    return static_cast<int>(length);
}
} // namespace croco