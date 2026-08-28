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
    bool _isValid(const unsigned short posid, const std::string &word);
    bool _isStopWord(const std::vector<std::string> &words);
    bool _isAllShortWords(const std::vector<std::string> &words, size_t minimum_word_size);
    bool _isSymbolWord(const std::string &word);
    bool _isSymbolCodePoint(const uint32_t code);
    bool _filtering(const phrase_t &phrase, int minimum_length = 3, size_t minimum_word_size = 2, size_t maximum_word_number=5);
    bool _appendMap(std::unordered_map<std::string, phrase_t> &map, Lines::line_t &line, std::vector<size_t> &idxs);
    void _insertkeys(std::vector<std::string> &keys, std::vector<std::string> &words, std::vector<size_t> &idxs);
    size_t _utf8Strlen(const std::string word);
    uint32_t _utf8CodePoint(const std::string &word, size_t &pos);
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
            if (_isValid(line.pos.at(idx), line.words.at(idx))) {
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
 * 品詞 ID だけでなく表層も見る。mecab は辞書に無い記号を未知語として推定し
 * 「名詞,サ変接続」(posid 36) を振るため、品詞 ID だけだと `(` `)。` `:` が
 * 内容語として名詞列に取り込まれ、`後述)。` `食文化概論:日本` `例).2016年度`
 * のような候補になる。全角記号は ipadic に「記号,括弧開」等で載っているので
 * この経路に入らないが、呼び出し側で NFKC 正規化した入力は半角に落ちて入る (#7)
 *
 * @access private
 * @param  const unsigned short posid
 * @param  const std::string &word
 * @return bool
 */
inline bool Phrases::_isValid(const unsigned short posid, const std::string &word)
{
    /* 名詞 36〜67 形容詞 10〜12 */
    if (36 > posid || 67 < posid) {
        if (10 > posid || 12 < posid) {
            return false;
        }
    }

    /* 表層の走査は品詞で絞ってから（大半の語はここへ来ない） */
    return !_isSymbolWord(word);
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
 * 全語が最小語長未満かの判定
 *
 * 1 語でも minimum_word_size 以上の語があれば false（＝フレーズを残す）。
 *
 * 以前は「1 語でも minimum_word_size 未満なら落とす」だった。pke の
 * candidate_filtering(minimum_word_size=2) をそのまま持ってきた条件で、1 文字語が
 * ノイズになる英語向けのもの。日本語の形態素に当てると、ipadic が `調理師免許` を
 * `調理 / 師 / 免許` と切るので 1 文字の接尾辞「師」だけでフレーズ全体が落ち、
 * `調理師` `栄養士` `食文化` のような接尾辞つき・1 文字語つきの複合名詞が
 * 候補から丸ごと消えていた (#7)
 *
 * フレーズ全体の長さは _filtering の minimum_length で見ているので、ここに残す
 * 役目は「1 文字語の切れ端だけでできた列」を落とすことだけでよい
 *
 * @access private
 * @param  const std::vector<std::string> &words
 * @param  size_t minimum_word_size
 * @return bool
 */
inline bool Phrases::_isAllShortWords(const std::vector<std::string> &words, size_t minimum_word_size)
{
    for (auto &word : words) {
        if (_utf8Strlen(word) >= minimum_word_size) {
            return false;
        }
    }

    return true;
}

/**
 * 記号だけで構成された語の判定
 *
 * @access private
 * @param  const std::string &word
 * @return bool
 */
inline bool Phrases::_isSymbolWord(const std::string &word)
{
    if (word.empty()) {
        return true;
    }

    for (size_t pos = 0; pos < word.size();) {
        if (!_isSymbolCodePoint(_utf8CodePoint(word, pos))) {
            return false;
        }
    }

    return true;
}

/**
 * 記号のコードポイント判定
 *
 * 未知語として名詞扱いされた記号を弾くためのもの。_isSymbolWord は「語を構成する
 * 文字が全部これ」のときだけ記号語と判定するので、々 や長音符 ー のように語の一部に
 * なりうる文字をここに含めても実在の語は落ちない（サーバー・人々には他の文字が
 * 混じるため）。逆に、それらだけでできた語は解析ノイズなので落として構わない。
 * 判定に漏れた記号があっても、単独なら minimum_length で落ちる
 *
 * @access private
 * @param  const uint32_t code
 * @return bool
 */
inline bool Phrases::_isSymbolCodePoint(const uint32_t code)
{
    if (0x80 > code) {
        /* ASCII は英数字以外（記号・空白・制御文字）を記号とみなす */
        return !((0x30 <= code && 0x39 >= code) ||
                 (0x41 <= code && 0x5A >= code) ||
                 (0x61 <= code && 0x7A >= code));
    }

    /* 一般句読点 (– — ‘ ’ “ ” … ‰ ′ ″ ほか) */
    if (0x2000 <= code && 0x206F >= code) {
        return true;
    }
    /* 矢印・数学記号・囲み文字・罫線・幾何学模様・その他の記号 */
    if (0x2190 <= code && 0x27BF >= code) {
        return true;
    }
    /* CJK の記号と句読点 (、 。 〈 〉 《 》 「 」 『 』 【 】 〜 々 〆 ほか) */
    if (0x3000 <= code && 0x303F >= code) {
        return true;
    }
    /* ・ (U+30FB) と長音符 ー (U+30FC) */
    if (0x30FB <= code && 0x30FC >= code) {
        return true;
    }
    /* 全角の記号 (！〜／ ：〜＠ ［〜｀ ｛〜･) */
    if (0xFF01 <= code && 0xFF0F >= code) {
        return true;
    }
    if (0xFF1A <= code && 0xFF20 >= code) {
        return true;
    }
    if (0xFF3B <= code && 0xFF40 >= code) {
        return true;
    }
    if (0xFF5B <= code && 0xFF65 >= code) {
        return true;
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
    } else if (_isAllShortWords(phrase.words, minimum_word_size)) {
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

/**
 * utf8コードポイントの取得（pos を次の文字へ進める）
 *
 * @access private
 * @param  const std::string &word
 * @param  size_t &pos
 * @return uint32_t
 */
inline uint32_t Phrases::_utf8CodePoint(const std::string &word, size_t &pos)
{
    uint8_t head = static_cast<uint8_t>(word[pos]);
    size_t size = (head < 0x80) ? 1 :
                  (head < 0xE0) ? 2 :
                  (head < 0xF0) ? 3 : 4;

    /* 途中で切れたバイト列で末尾を踏み越えないよう、残りバイト数で頭打ちにする */
    if (size > word.size() - pos) {
        size = word.size() - pos;
    }

    uint32_t code = (1 == size) ? head :
                    (2 == size) ? (head & 0x1F) :
                    (3 == size) ? (head & 0x0F) : (head & 0x07);
    for (size_t idx = 1; idx < size; idx++) {
        code = (code << 6) | (static_cast<uint8_t>(word[pos + idx]) & 0x3F);
    }
    pos += size;

    return code;
}

} // namespace croco