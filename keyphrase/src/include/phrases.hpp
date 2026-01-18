#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "lines.hpp"

namespace croco {

/**
 * フレーズ抽出（topc rank）クラス
 *
 * @package     keyphrase
 * @author      Yujiro Takahashi <yujiro@cro-co.co.jp>
 */
class Phrases : public Lines {
public:
    typedef struct _phrase_t {
        std::string key;
        std::vector<std::string> words;
        std::vector<float> offsets;
        int length;
        void line_t() {
            clear();
        }
        void clear() {
            key.assign("");
            words.clear();
            offsets.clear();
            length = 0;
        }
    } phrase_t;

    typedef struct _candidate_t {
        std::vector<std::string> keys;
        std::unordered_map<std::string, phrase_t> map;
    } candidate_t;

public:
    candidate_t parse(std::vector<Lines::line_t> lines);

private:
    bool _isValid(const unsigned short posid);
    bool _isStopWord(const std::vector<std::string> &words);
    bool _isMinimumWordSize(const std::vector<std::string> &words, size_t minimum_word_size);
    bool _filtering(const phrase_t &phrase, int minimum_length = 3, size_t minimum_word_size = 2, size_t maximum_word_number=5);
    bool _appendMap(std::unordered_map<std::string, phrase_t> &map, Lines::line_t &line, std::vector<size_t> &idxs);
    void _insertkeys(std::vector<std::string> &keys, std::vector<std::string> &words, std::vector<size_t> &idxs);
    size_t _utf8Strlen(const std::string word);
}; // class Lines

/**
 * トピック（フレーズ）の取得
 *
 * @access public
 * @param  std::vector<Lines::line_t> lines
 * @return std::vector<std::string>
 */
inline Phrases::candidate_t Phrases::parse(std::vector<Lines::line_t> lines)
{
    std::vector<std::string> allKeys;
    std::unordered_map<std::string, phrase_t> map;
    
    for (auto &line : lines) {
        size_t old = 0;
        std::vector<size_t> idxs;
        for (size_t idx = 0; idx < line.pos.size(); idx++) {
            if (_isValid(line.pos.at(idx))) {
                if (old != (idx - 1)) {
                    if (idxs.size()) {
                        if (!_appendMap(map, line, idxs)) {
                            _insertkeys(allKeys, line.words, idxs);
                        };
                    }
                    idxs.clear();
                }
                idxs.push_back(idx);
                old = idx;
            }
        }
        if (idxs.size()) {
            if (!_appendMap(map, line, idxs)) {
                _insertkeys(allKeys, line.words, idxs);
            };
        }
    }

    std::vector<std::string> keys;
    candidate_t result;
    for (auto &key : allKeys) {
        auto &phrase = map.at(key);
        if (_filtering(phrase)) {
            result.map.insert(std::make_pair(key, phrase));
            result.keys.push_back(key);
        }
    }

    /**
     * フレーズが取得出来ない場合、フィルタリングの緩和
     * */
    if (0 == result.keys.size()) {
        for (auto &key : allKeys) {
            auto &phrase = map.at(key);
            if (_filtering(phrase, 1)) {
                result.map.insert(std::make_pair(key, phrase));
                result.keys.push_back(key);
            }
        }
    } // if (0 == result.size())

    return result;
}

/**
 * NOUN PROPN ADJ NUM 判定
 *
 * @access private
 * @param  const unsigned short posid
 * @return bool
 */
inline bool Phrases::_isValid(const unsigned short posid)
{
    /* 名詞 36〜67 形容詞 10〜12 */
    if (36 <= posid && 67 >= posid) {
        return true;
    } else if (10 <= posid && 12 >= posid) {
        return true;
    }
    return false;
}

/**
 * ストップワードの 判定
 *
 * @access private
 * @param  std::vector<std::string> words
 * @return bool
 */
inline bool Phrases::_isStopWord(const std::vector<std::string> &words)
{
    std::vector<std::string> stopwors = {
        "より", "らしい", "いう", "ず", "なし", "せ", "たい", "でき", "さらに", "のみ", 
        "き", "れ", "を", "また", "か", "もと", "ちゃん", "べき", "せい", "す", "ほとんど", 
        "ば", "ぬ", "ま", "ごと", "れる", "ね", "かけ", "ん", "とっ", "られる", "さ", 
        "そして", "その", "え", "お", "つけ", "ながら", "一", "だけ", "ない", "の", "よ", 
        "なる", "くる", "それ", "そこ", "は", "にて", "いっ", "なら", "それぞれ", "し", 
        "とき", "どう", "なく", "よれ", "すぐ", "いわ", "て", "いずれ", "そう", "くん", 
        "ここ", "ます", "しか", "へ", "のち", "かつ", "もの", "せる", "な", "いい", "ご", 
        "あれ", "つつ", "きっかけ", "る", "できる", "ただし", "いく", "です", "なけれ", 
        "なかっ", "たち", "もっ", "が", "こと", "つい", "ため", "と", "た", "もう", "や", 
        "よる", "この", "これ", "られ", "あまり", "しよう", "ち", "だ", "ほど", "ところ", 
        "うち", "から", "ら", "かつて", "よう", "および", "こ", "まで", "い", "たり", "なお", 
        "も", "ほか", "さん", "とも", "ひと", "み", "なっ", "いる", "おり", "よっ", "おい", 
        "おけ", "つ", "ある", "いつ", "よく", "おら", "ぶり", "しかし", "しまう", "あるいは", 
        "だっ", "やっ", "はじめ", "すべて", "かなり", "こう", "など", "ほぼ", "に", "まま", 
        "たら", "しまっ", "あっ", "あ", "で", "する", "あり", "なり"
    };

    for (auto &word : words) {
        for (auto &stop : stopwors) {
            if (word.compare(stop) == 0) {
                return true;
            }
        }
    }

    return false;
}

/**
 * 最小値判定
 *
 * @access private
 * @param  const std::vector<std::string> &words
 * @param  int minimum_word_size
 * @return bool
 */
inline bool Phrases::_isMinimumWordSize(const std::vector<std::string> &words, size_t minimum_word_size)
{
    for (auto &word : words) {
        if (_utf8Strlen(word) < minimum_word_size) {
            return true;
        }
    }

    return false;
}

/**
 * フィルタリング
 *
 * @access private
 * @param const phrase_t &phrase
 * @param const int minimum_length
 * @param const int minimum_word_size
 * @param const int maximum_word_number
 * @return bool
 */
inline bool Phrases::_filtering(const phrase_t &phrase, int minimum_length, size_t minimum_word_size, size_t maximum_word_number)
{
    if (_isStopWord(phrase.words)) {
        return false;
    } else if (minimum_length > phrase.length) {
        return false;
    } else if (_isMinimumWordSize(phrase.words, minimum_word_size)) {
        return false;
    } else if (phrase.words.size() > maximum_word_number) {
        return false;
    }

    return true;
}

/**
 * shiftマップの作成
 *
 * @access private
 * @param  std::unordered_map<std::string, phrase_t> &map
 * @param  Lines::line_t &line
 * @param  std::vector<size_t> &idxs
 * @return bool
 */
inline bool Phrases::_appendMap(std::unordered_map<std::string, phrase_t> &map, Lines::line_t &line, std::vector<size_t> &idxs)
{
    std::string key("");
    for (auto &idx : idxs) {
        key.append(line.words.at(idx));
    }

    if (map.find(key) == map.end()) {
        phrase_t phrase;
        for (auto &idx : idxs) {
            phrase.words.push_back(line.words.at(idx));
        }
        phrase.key.assign(key);
        phrase.offsets.push_back(line.shift + idxs.at(0));
        phrase.length = _utf8Strlen(key);
        map.insert(std::make_pair(key, phrase));

        return false;
    } else {
        map.at(key).offsets.push_back(line.shift + idxs.at(0));
    }
    return true;
}

/**
 * キーリスト（フレーズ）の追加
 *
 * @access private
 * @param  std::vector<std::string> &keys
 * @param  std::vector<std::string> &words
 * @param  std::vector<size_t> &idxs
 * @return void
 */
inline void Phrases::_insertkeys(std::vector<std::string> &keys, std::vector<std::string> &words, std::vector<size_t> &idxs)
{
    std::string key("");
    for (auto &idx : idxs) {
        key.append(words.at(idx));
    }
    keys.push_back(key);
}

/**
 * utf8文字数カウンタ
 *
 * @access private
 * @param  const std::string word
 * @return int
 */
inline size_t Phrases::_utf8Strlen(const std::string word)
{
    size_t length = 0;
    for (size_t pos = 0; pos < word.size();) {
        uint8_t hex = static_cast<uint8_t>(word[pos]);
        pos += (hex < 0x80) ? 1 :
               (hex < 0xE0) ? 2 :
               (hex < 0xF0) ? 3 : 4;
        length += 1;
    }

    return length;
}

} // namespace croco