<?php
declare(strict_types=1);
// add() / addWithIds() / search() の受け入れテスト（issue #1 / PR #2 の回帰）
// 実行例: php -d extension=/path/to/croco_faiss.so tests/test_faiss.php
require __DIR__ . '/helper.php';
requireFaiss();

const DIM = 4;

// ── add ──
$idx = new \Croco\Faiss(DIM);
$idx->add(array_fill(0, DIM, 0.1));
ok($idx->ntotal() === 1, 'add: 1 ベクトル投入で ntotal = 1');

throws(fn() => $idx->add([]), 'add: 空配列は例外 (issue #1-2)',
    ErrorException::class, 'positive multiple');
throws(fn() => $idx->add([0.1, 0.2]), 'add: 要素数が次元の倍数でない配列は例外 (issue #1-2)',
    ErrorException::class, 'positive multiple');
throws(fn() => $idx->add(array_fill(0, DIM, 'x')), 'add: 文字列要素は例外',
    ErrorException::class, 'must be int or float');
throws(fn() => $idx->add(array_fill(0, DIM, 0.1), 2), 'add: number 不一致は例外',
    ErrorException::class, 'does not match');
// (2^62 + 1) * 4 = 2^64 + 4 が size_t で 4 に wrap し、要素数 4 を偽装できた値
throws(fn() => $idx->add(array_fill(0, DIM, 0.1), intdiv(PHP_INT_MAX, 2) + 2),
    'add: number の乗算 wrap による検証迂回は例外',
    ErrorException::class, 'does not match');
ok($idx->ntotal() === 1, 'add: 失敗した投入で ntotal が変わらない');

$idx->add(array_fill(0, DIM, 1)); // int 要素は float に変換して許容
ok($idx->ntotal() === 2, 'add: int 要素の配列も投入できる');

// ── search ──
throws(fn() => $idx->search([0.1]), 'search: 要素数 < 次元は例外 (issue #1-1a)');

$rows = $idx->search(array_fill(0, DIM, 0.1), 2);
ok(is_array($rows) && count($rows) >= 1, 'search: 結果が返る');
ok(isset($rows[0]['Rank'], $rows[0]['ID'], $rows[0]['Count'], $rows[0]['Distance']),
    'search: Rank/ID/Count/Distance キーを持つ');

// 2 行クエリ × k=2 → n * k = 4 件が書き込まれ、ラベル単位に集約されて
// 2 件（各 Count = 2）になる。k 件しか確保しない旧実装ではここが壊れる
$rows = $idx->search(array_fill(0, DIM * 2, 0.1), 2);
ok(is_array($rows) && count($rows) === 2, 'search: 2 行クエリ (n = 2) で 2 ラベルが返る (issue #1-1b)');
ok(($rows[0]['Count'] ?? null) === 2, 'search: n = 2 の結果はクエリ横断で集約され Count = 2');
throws(fn() => $idx->search(array_fill(0, DIM, 0.1), 1, 0, 2), 'search: number 不一致は例外',
    ErrorException::class, 'does not match');

// 打ち切り回帰 (issue #1-3): 5 件登録して k = 5 で 5 件返る
$idx5 = new \Croco\Faiss(DIM);
for ($i = 0; $i < 5; $i++) {
    $idx5->add([$i * 1.0, 0.0, 0.0, 0.0]);
}
$rows = $idx5->search([0.0, 0.0, 0.0, 0.0], 5);
ok(count($rows) === 5, 'search: k = ntotal = 5 で 5 件すべて返る (issue #1-3)');

$rows = $idx5->search([0.0, 0.0, 0.0, 0.0], 100);
ok(count($rows) === 5, 'search: k > ntotal は ntotal 件に丸まる');
ok(!in_array(-1, array_column($rows, 'ID'), true), 'search: 番兵 -1 が結果に混ざらない');

$empty = new \Croco\Faiss(DIM);
ok($empty->search(array_fill(0, DIM, 0.1)) === [], 'search: 空インデックス (k 省略) は空配列 (issue #1-1c)');
ok($empty->search(array_fill(0, DIM, 0.1), 10) === [], 'search: 空インデックス (k 明示) は空配列');

// ── addWithIds ──
$ids = new \Croco\Faiss(DIM, 'IDMap,Flat');
$ids->addWithIds(array_fill(0, DIM, 0.5), [100]);
ok($ids->ntotal() === 1, 'addWithIds: 投入できる');
throws(fn() => $ids->addWithIds(array_fill(0, DIM, 0.5), [1, 2]), 'addWithIds: ids 件数不一致は例外');
throws(fn() => $ids->addWithIds(array_fill(0, DIM, 0.5), [-1]), 'addWithIds: 予約 ID -1 は例外');
throws(fn() => $ids->addWithIds(array_fill(0, DIM, 0.5), ['x']), 'addWithIds: int 以外の ID は例外');

$ids->addWithIds(array_fill(0, DIM, 0.9), [-5]); // -1 以外の負 ID は許容
$rows = $ids->search(array_fill(0, DIM, 0.9), 1);
ok(($rows[0]['ID'] ?? null) === -5, 'addWithIds: 負 ID (-5) は登録でき検索で返る');

// ── dimension 0 ──
// d = 0 の構築可否は faiss のバージョン次第。どちらでも SIGFPE にならないことを固定する
try {
    $zero = new \Croco\Faiss(0);
    throws(fn() => $zero->add([1.0]), 'dimension 0: add は例外になる（SIGFPE でプロセスごと落ちない）',
        ErrorException::class, 'dimension must be positive');
} catch (Throwable $e) {
    ok($e instanceof ErrorException, 'dimension 0: コンストラクタ自体が例外（faiss 側で拒否）');
}

finish();
