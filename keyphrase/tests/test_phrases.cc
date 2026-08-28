// phrases.hpp（候補フレーズの切り出しとフィルタ）のホスト単体テスト（zend / mecab 非依存）
//
// ビルド・実行:
//   g++ -std=c++17 -Wall -Wextra -I../include -o test_phrases test_phrases.cc && ./test_phrases
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "phrases.hpp"

// ipadic の posid（pos-id.def より）
static const unsigned short PARTICLE = 13;          // 助詞,格助詞,一般
static const unsigned short NOUN_SAHEN = 36;        // 名詞,サ変接続
static const unsigned short NOUN = 38;              // 名詞,一般
static const unsigned short NOUN_PROPER = 41;       // 名詞,固有名詞,一般
static const unsigned short NOUN_PROPER_AREA = 46;  // 名詞,固有名詞,地域,一般
static const unsigned short NOUN_NUMBER = 48;       // 名詞,数
static const unsigned short NOUN_SUFFIX = 51;       // 名詞,接尾,一般
static const unsigned short NOUN_COUNTER = 53;      // 名詞,接尾,助数詞
static const unsigned short NOUN_DEPENDENT = 63;    // 名詞,非自立,一般

static croco::Lines::line_t makeLine(const std::vector<std::string> &words,
                                     const std::vector<unsigned short> &pos)
{
    croco::Lines::line_t line;
    line.words = words;
    line.pos = pos;
    line.shift = 0;
    return line;
}

// 上限と無関係なケース用。既定の上限（MAXIMUM_CANDIDATES）を明示して渡す
static std::vector<std::string> keysOf(const std::vector<croco::Lines::line_t> &lines)
{
    croco::Phrases phrases;
    return phrases.parse(lines, croco::Phrases::MAXIMUM_CANDIDATES).keys;
}

static std::vector<std::string> keysOf(const std::vector<croco::Lines::line_t> &lines, size_t maximum)
{
    croco::Phrases phrases;
    return phrases.parse(lines, maximum).keys;
}

static bool has(const std::vector<std::string> &keys, const std::string &key)
{
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

int main() {
    // ── 1 文字の形態素を含む複合名詞が候補に残る（#7 の本体）──
    // ipadic は 調理師免許 を 調理 / 師 / 免許 と切る。旧実装は 1 文字の「師」だけで
    // フレーズ全体を捨てていた
    {
        auto keys = keysOf({makeLine({"調理", "師", "免許", "の", "取得"},
                                     {NOUN_SAHEN, NOUN_SUFFIX, NOUN_SAHEN, PARTICLE, NOUN_SAHEN})});
        assert(has(keys, "調理師免許"));
        assert(keys.size() == 1);   // 取得（2 文字）は minimum_length で落ちる
    }

    // 先頭が 1 文字語でも同じ（食 + 文化）
    {
        auto keys = keysOf({makeLine({"食", "文化", "の", "歴史"},
                                     {NOUN, NOUN, PARTICLE, NOUN})});
        assert(has(keys, "食文化"));
    }

    // ── 1 文字語だけでできた列は落ちる（minimum_word_size の残す役目）──
    // 3 文字あるので minimum_length は通るが、2 文字以上の語が 1 つも無い
    {
        auto keys = keysOf({makeLine({"上", "中", "下"}, {NOUN, NOUN, NOUN})});
        assert(keys.empty());
    }

    // ── 記号は内容語として扱わず、名詞列を分断する ──
    // mecab は辞書に無い記号を未知語として「名詞,サ変接続」と推定するため、
    // posid だけを見ていると記号ごと 1 フレーズに繋がる
    {
        // NFKC 正規化で半角に落ちたコロン
        auto keys = keysOf({makeLine({"食品衛生", ":", "調理理論"},
                                     {NOUN_SAHEN, NOUN_SAHEN, NOUN_SAHEN})});
        assert(keys.size() == 2);
        assert(has(keys, "食品衛生"));
        assert(has(keys, "調理理論"));
        assert(!has(keys, "食品衛生:調理理論"));
    }
    {
        // 半角と全角が 1 語にまとまった未知語（例: `後述)。`）
        auto keys = keysOf({makeLine({"食品衛生", ")。", "調理理論"},
                                     {NOUN_SAHEN, NOUN_SAHEN, NOUN_SAHEN})});
        assert(keys.size() == 2);
        assert(!has(keys, "食品衛生)。調理理論"));
    }
    {
        // 全角記号・中黒・括弧・絵文字（4 バイト UTF-8）
        for (const std::string symbol : {"・", "、", "（", "〜", "／", "…", "😀", "★"}) {
            auto keys = keysOf({makeLine({"食品衛生", symbol, "調理理論"},
                                         {NOUN_SAHEN, NOUN_SAHEN, NOUN_SAHEN})});
            assert(keys.size() == 2);
        }
    }
    {
        // 漢字・かな・英数字は記号ではない（分断しない）
        for (const std::string word : {"上", "ノ", "2016", "ab"}) {
            auto keys = keysOf({makeLine({"食品衛生", word, "調理理論"},
                                         {NOUN_SAHEN, NOUN, NOUN_SAHEN})});
            assert(keys.size() == 1);
            assert(has(keys, "食品衛生" + word + "調理理論"));
        }
    }

    // ── 既存のフィルタは効いたまま ──
    {
        // ストップワード（こと）を含むフレーズは落ちる
        auto keys = keysOf({makeLine({"調理", "こと"}, {NOUN_SAHEN, NOUN_DEPENDENT})});
        assert(keys.empty());
    }
    {
        // maximum_word_number（既定 5）を超える列は落ちる
        auto keys = keysOf({makeLine({"衛生", "管理", "責任", "業務", "規定", "基準"},
                                     {NOUN_SAHEN, NOUN_SAHEN, NOUN, NOUN_SAHEN, NOUN, NOUN})});
        assert(keys.empty());
    }
    {
        // 助詞で名詞列が分断される
        auto keys = keysOf({makeLine({"調理", "師", "の", "免許証"},
                                     {NOUN_SAHEN, NOUN_SUFFIX, PARTICLE, NOUN})});
        assert(keys.size() == 2);
        assert(has(keys, "調理師"));
        assert(has(keys, "免許証"));
    }

    // ── 候補が 0 件のときの緩和パス（minimum_length = 1）──
    {
        // 免許（2 文字）は通常のフィルタでは落ちるが、他に候補が無ければ拾われる
        auto keys = keysOf({makeLine({"免許", "を"}, {NOUN_SAHEN, PARTICLE})});
        assert(keys.size() == 1);
        assert(has(keys, "免許"));
    }
    {
        // 緩和しても記号語・ストップワードは拾わない
        auto keys = keysOf({makeLine({"(", ")", "。"}, {NOUN_SAHEN, NOUN_SAHEN, NOUN_SAHEN})});
        assert(keys.empty());
    }

    // ── 同じフレーズが複数回出てもキーは 1 つ ──
    {
        auto keys = keysOf({
            makeLine({"調理", "師", "免許"}, {NOUN_SAHEN, NOUN_SUFFIX, NOUN_SAHEN}),
            makeLine({"調理", "師", "免許"}, {NOUN_SAHEN, NOUN_SUFFIX, NOUN_SAHEN}),
        });
        assert(keys.size() == 1);
        assert(has(keys, "調理師免許"));
    }

    // ── 固有名詞・数詞も内容語として扱う（既存挙動）──
    {
        auto keys = keysOf({makeLine({"関西", "広域", "連合"},
                                     {NOUN_PROPER, NOUN_SAHEN, NOUN_SAHEN})});
        assert(has(keys, "関西広域連合"));
    }

    // ── Lines::parse の shift が行をまたいで累積する ──
    // shift は「先行行の語数の累積」＝語の大域インデックス。_getWeight の gap は
    // これを使うので、行の切り方が細かくなっても値が変わらないことが前提になる
    {
        std::vector<std::vector<std::string>> wordLines = {
            {"調理", "師"},                 // 2 語
            {"免許", "の", "取得"},          // 3 語
            {"試験"},                        // 1 語
        };
        std::vector<std::vector<unsigned short>> posLines = {
            {NOUN_SAHEN, NOUN_SUFFIX},
            {NOUN_SAHEN, PARTICLE, NOUN_SAHEN},
            {NOUN_SAHEN},
        };
        croco::Lines lineParser;
        auto lines = lineParser.parse(wordLines, posLines);

        assert(lines.size() == 3);
        assert(lines.at(0).shift == 0.0f);
        assert(lines.at(1).shift == 2.0f);   // 1 行目の 2 語ぶん
        assert(lines.at(2).shift == 5.0f);   // 1〜2 行目の 5 語ぶん
        assert((lines.at(1).words == std::vector<std::string>{"免許", "の", "取得"}));
        assert((lines.at(1).pos
                == std::vector<unsigned short>{NOUN_SAHEN, PARTICLE, NOUN_SAHEN}));

        // フレーズの offsets は shift + 行内の位置（＝大域インデックス）になる
        croco::Phrases phrases;
        auto cand = phrases.parse(lines, croco::Phrases::MAXIMUM_CANDIDATES);
        assert(cand.map.count("調理師") == 1);
        assert(cand.map.at("調理師").offsets.at(0) == 0.0f);
        assert(cand.map.count("取得試験") == 0);   // 行をまたいで繋がらない
    }
    {
        // 空行があっても累積はずれない
        std::vector<std::vector<std::string>> wordLines = {{"調理", "師"}, {}, {"免許証"}};
        std::vector<std::vector<unsigned short>> posLines = {
            {NOUN_SAHEN, NOUN_SUFFIX}, {}, {NOUN},
        };
        croco::Lines lineParser;
        auto lines = lineParser.parse(wordLines, posLines);
        assert(lines.at(1).shift == 2.0f);
        assert(lines.at(2).shift == 2.0f);
    }

    // ── 数量だけのフレーズは落とす ──
    // mecab は 3,000,000円 を 3 / , / 000 / , / 000 / 円 と切り、`,` は未知語なので
    // 記号として分断される。残った `000円` のような断片を候補にしない
    {
        auto keys = keysOf({makeLine({"000", "円"}, {NOUN_NUMBER, NOUN_COUNTER})});
        assert(keys.empty());
    }
    {
        // 2016年度 / 30万円 も同じ（数詞と助数詞だけ）
        auto keys = keysOf({makeLine({"2016", "年度"}, {NOUN_NUMBER, NOUN_COUNTER})});
        assert(keys.empty());
        keys = keysOf({makeLine({"30", "万", "円"}, {NOUN_NUMBER, NOUN_NUMBER, NOUN_COUNTER})});
        assert(keys.empty());
    }
    {
        // 数詞だけの語も落ちる（緩和パスでも拾わない）
        auto keys = keysOf({makeLine({"100"}, {NOUN_NUMBER})});
        assert(keys.empty());
    }
    {
        // 内容語が 1 つでも混じっていれば残す（地名・規格として意味を持つ）
        auto keys = keysOf({makeLine({"国道", "246", "号"},
                                     {NOUN, NOUN_NUMBER, NOUN_SUFFIX})});
        assert((keys == std::vector<std::string>{"国道246号"}));

        keys = keysOf({makeLine({"笹塚", "1", "丁目"},
                                {NOUN_PROPER_AREA, NOUN_NUMBER, NOUN_COUNTER})});
        assert((keys == std::vector<std::string>{"笹塚1丁目"}));

        keys = keysOf({makeLine({"60", "mg"}, {NOUN_NUMBER, NOUN})});
        assert((keys == std::vector<std::string>{"60mg"}));
    }
    {
        // 数量が名詞列の途中に挟まっても、内容語があれば分断せず残す
        auto keys = keysOf({makeLine({"受験", "者", "3204", "人"},
                                     {NOUN_SAHEN, NOUN_SUFFIX, NOUN_NUMBER, NOUN_COUNTER})});
        assert((keys == std::vector<std::string>{"受験者3204人"}));
    }

    // ── 候補数の上限 ──
    // 下流の MultipartiteRank が完全グラフを張るので、候補数は n^2 で効く。
    // 上限を超えたら出現回数の多い順に残し、残した集合は初出順に並べ直す
    {
        // 初出順は 調理師免許 → 食品衛生 → 専門学校、出現回数は 3 / 1 / 2
        const std::vector<croco::Lines::line_t> lines = {
            makeLine({"調理", "師", "免許"}, {NOUN_SAHEN, NOUN_SUFFIX, NOUN_SAHEN}),
            makeLine({"食品衛生"}, {NOUN_SAHEN}),
            makeLine({"専門学校"}, {NOUN}),
            makeLine({"調理", "師", "免許"}, {NOUN_SAHEN, NOUN_SUFFIX, NOUN_SAHEN}),
            makeLine({"専門学校"}, {NOUN}),
            makeLine({"調理", "師", "免許"}, {NOUN_SAHEN, NOUN_SUFFIX, NOUN_SAHEN}),
        };

        // 上限に掛からなければ中身も並びも変わらない
        assert((keysOf(lines) == std::vector<std::string>{"調理師免許", "食品衛生", "専門学校"}));
        assert((keysOf(lines, 3) == std::vector<std::string>{"調理師免許", "食品衛生", "専門学校"}));
        assert((keysOf(lines, 99) == std::vector<std::string>{"調理師免許", "食品衛生", "専門学校"}));
        // NO_CANDIDATE_LIMIT（0）は無制限
        assert((keysOf(lines, croco::Phrases::NO_CANDIDATE_LIMIT)
                == std::vector<std::string>{"調理師免許", "食品衛生", "専門学校"}));

        // 出現 1 回の 食品衛生 が落ち、残りは初出順のまま
        assert((keysOf(lines, 2) == std::vector<std::string>{"調理師免許", "専門学校"}));
        assert((keysOf(lines, 1) == std::vector<std::string>{"調理師免許"}));
    }
    {
        // 出現回数が同じなら初出の早いほうを残す
        const std::vector<croco::Lines::line_t> lines = {
            makeLine({"食品衛生"}, {NOUN_SAHEN}),
            makeLine({"専門学校"}, {NOUN}),
        };
        assert((keysOf(lines, 1) == std::vector<std::string>{"食品衛生"}));
    }

    puts("all tests passed");
    return 0;
}
