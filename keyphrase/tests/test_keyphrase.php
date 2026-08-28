<?php
declare(strict_types=1);
// 候補フレーズの切り出しの受け入れテスト（issue #7 の回帰）
// 実行例: php -d extension=/path/to/croco_keyphrase.so tests/test_keyphrase.php
//
// mecab の分かち書きに依存するため、ホスト単体テスト（tests/test_phrases.cc）では
// 代替できない。実際の posid・未知語推定・行の切り方を通した結果を見る
require __DIR__ . '/helper.php';
requireKeyphrase();

$keyphrase = new \Croco\Keyphrase(dicPath());

// ── 1 文字の形態素を含む複合名詞が候補に残る ──
// ipadic は 調理師免許 を 調理 / 師 / 免許 と切る。1 文字の接尾辞「師」を理由に
// フレーズ全体が捨てられていた (#7)
$candidates = $keyphrase->candidate('調理師免許の取得には調理師試験の合格が必要になる。');
ok(in_array('調理師免許', $candidates, true), '1 文字の接尾辞を含む 調理師免許 が候補に残る');
ok(in_array('調理師試験', $candidates, true), '同じく 調理師試験 が候補に残る');

// ── 行末でセンテンスが切れる ──
// 句点で終わらない見出し行が次の行の本文と 1 センテンスに繋がり、見出し末尾と
// 本文先頭がくっついた候補（勉強コスト独学）が生まれていた (#7)
$candidates = $keyphrase->candidate("勉強コスト\n独学の場合、テキストや参考書費用で1万円程度。\n");
ok(in_array('勉強コスト', $candidates, true), '見出し行が単独の候補になる');
ok(!in_array('勉強コスト独学', $candidates, true), '見出し行と次行の本文が連結した候補は作らない');

// 1 行の中では句点まで従来どおり繋がる（行末での分割が文中の分割になっていない）
$candidates = $keyphrase->candidate('調理師免許の申請先は各都道府県の保健所。');
ok(in_array('調理師免許', $candidates, true), '行内の解析は従来どおり');

// ── 記号は内容語として扱わない ──
// mecab は辞書に無い半角記号を未知語として「名詞,サ変接続」と推定する。
// NFKC 正規化を通した入力は全角記号が半角に落ちるのでこの経路に入る (#7)
$candidates = $keyphrase->candidate('食文化概論:調理師の社会的役割について。');
ok(in_array('食文化概論', $candidates, true), '記号の手前が候補になる');
ok(!in_array('食文化概論:調理師', $candidates, true), '半角記号をまたいで名詞列が繋がらない');

$candidates = $keyphrase->candidate('一定の条件さえ満たしていれば誰でも取得が可能(詳細は後述)。調理師免許は有効期限がない。');
$joined = array_values(array_filter($candidates, static fn ($c) => str_contains($c, ')')));
ok($joined === [], '未知語の記号 `)。` を含む候補が残らない: ' . json_encode($joined, JSON_UNESCAPED_UNICODE));

// ── 数量だけのフレーズは候補にしない ──
// mecab は 3,000,000円 を 3 / , / 000 / , / 000 / 円 と切り、`,` は辞書に無いので
// 未知語（名詞,サ変接続）になる。記号として分断された結果 `000円` が残っていた (#7)
$candidates = $keyphrase->candidate('開業資金は参考書費用も含めて3,000,000円かかる。');
$fragments = array_values(array_filter($candidates, static fn ($c) => str_contains($c, '000')));
ok($fragments === [], '桁区切りの金額から壊れた断片が残らない: ' . json_encode($fragments, JSON_UNESCAPED_UNICODE));
ok(in_array('開業資金', $candidates, true), '同じ文の内容語は候補に残る');

$candidates = $keyphrase->candidate('試験は2016年度から年1回の実施になった。');
ok(!in_array('2016年度', $candidates, true), '数詞と助数詞だけの 2016年度 は候補にしない');

// 内容語が 1 つでも混じっていれば残す（地名・規格として意味を持つ）。
// 名詞列のどこで切れるかは mecab 任せなので（`国道246号沿い` まで 1 語になる）、
// 境界そのものではなく「数量を含む候補が残ること」を見る
$candidates = $keyphrase->candidate('国道246号沿いの笹塚1丁目に出店する。');
$kept = array_values(array_filter($candidates, static fn ($c) => str_contains($c, '246')));
ok($kept !== [], '内容語混じりの 国道246号 は候補に残る: ' . json_encode($kept, JSON_UNESCAPED_UNICODE));
$kept = array_values(array_filter($candidates, static fn ($c) => str_contains($c, '丁目')));
ok($kept !== [], '内容語混じりの 笹塚1丁目 は候補に残る: ' . json_encode($kept, JSON_UNESCAPED_UNICODE));

// ── extract() まで通して主題語が返る ──
$text = '調理師免許は調理師法に定められた国家資格。'
      . "\n" . '調理師免許がなくても飲食店で料理を出すことは可能だが、調理師と名乗れるのは免許を持った者だけ。'
      . "\n" . '調理師免許の発行には調理技術技能センターが実施する調理師試験に合格する必要がある。';
$nodes = $keyphrase->extract($text);
$phrases = array_column($nodes, 'phrase');
ok(in_array('調理師免許', $phrases, true), 'extract() が主題語 調理師免許 を返す');
// 順位そのものは ipadic のバージョンと重み計算の両方に依存するので、
// 「主題語が上位に来る」ことだけを見る
$top3 = array_slice($phrases, 0, 3);
ok(in_array('調理師免許', $top3, true),
    'extract() の上位 3 件に 調理師免許 が入る（実際: ' . json_encode($top3, JSON_UNESCAPED_UNICODE) . '）');

finish();
