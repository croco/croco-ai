<?php
declare(strict_types=1);
// metric 別の並び順と arginfo 整合の受け入れテスト（issue #4 / PR #6 の回帰）
// 実行例: php -d extension=/path/to/croco_faiss.so tests/test_faiss_metric.php
require __DIR__ . '/helper.php';
requireFaiss();

const DIM = 4;

// ── METRIC_INNER_PRODUCT: 値が大きいほど類似 → Rank 1 が内積最大 ──
$ip = new \Croco\Faiss(DIM, 'Flat', \Croco\Faiss\METRIC_INNER_PRODUCT);
$ip->add([1.0, 0.0, 0.0, 0.0]);  // ID 0: 内積 1（最も似ていない）
$ip->add([10.0, 0.0, 0.0, 0.0]); // ID 1: 内積 10（最類似）
$ip->add([5.0, 0.0, 0.0, 0.0]);  // ID 2: 内積 5
$rows = $ip->search([1.0, 0.0, 0.0, 0.0], 3);
ok(($rows[0]['ID'] ?? null) === 1, 'METRIC_INNER_PRODUCT: Rank 1 = 内積最大 (ID 1)');
ok(($rows[2]['ID'] ?? null) === 0, 'METRIC_INNER_PRODUCT: 最下位 = 内積最小 (ID 0)');

// ── METRIC_L2: 値が小さいほど近い → Rank 1 が距離最小 ──
$l2 = new \Croco\Faiss(DIM);
$l2->add([0.0, 0.0, 0.0, 0.0]);  // ID 0: 距離 0（最近傍）
$l2->add([9.0, 0.0, 0.0, 0.0]);  // ID 1: 距離 81
$rows = $l2->search([0.0, 0.0, 0.0, 0.0], 2);
ok(($rows[0]['ID'] ?? null) === 0, 'METRIC_L2: Rank 1 = 距離最小 (ID 0)');

// ── arginfo 整合（Reflection がエラーなく解決できる）──
$params = (new ReflectionMethod(\Croco\Faiss::class, '__construct'))->getParameters();
ok($params[1]->getDefaultValue() === 'Flat',
    "arginfo: description のデフォルトが 'Flat'（未定義定数エラーにならない）");
ok($params[2]->getName() === 'metric', 'arginfo: ctor 第 3 引数の名前が metric');
ok($params[2]->getDefaultValue() === \Croco\Faiss\METRIC_L2, 'arginfo: metric のデフォルトが METRIC_L2');

$k = (new ReflectionMethod(\Croco\Faiss::class, 'search'))->getParameters()[1];
ok(!$k->getType()->allowsNull(), 'arginfo: search の k は非 nullable（実装と一致）');
ok($k->getDefaultValue() === 0, 'arginfo: search の k のデフォルトが 0');

finish();
