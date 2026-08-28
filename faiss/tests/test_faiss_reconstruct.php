<?php
declare(strict_types=1);
// reconstruct() の受け入れテスト（issue #3 / PR #5 の回帰)
// 実行例: php -d extension=/path/to/croco_faiss.so tests/test_faiss_reconstruct.php
require __DIR__ . '/helper.php';
requireFaiss();

const DIM = 4;

$idx = new \Croco\Faiss(DIM);
$vec = [0.25, 0.5, 0.75, 1.0]; // float32 で厳密表現できる値
$idx->add($vec);

$recons = $idx->reconstruct(0);
ok(is_array($recons) && count($recons) === DIM, 'reconstruct: 戻り値の要素数が次元と一致');

$match = true;
foreach ($vec as $i => $v) {
    if (abs($recons[$i] - $v) > 1e-6) {
        $match = false;
        break;
    }
}
ok($match, 'reconstruct: 投入したベクトルが復元される');

throws(fn() => $idx->reconstruct(100), 'reconstruct: 範囲外 key は例外');
throws(fn() => $idx->reconstruct(PHP_INT_MAX), 'reconstruct: PHP_INT_MAX は例外（faiss 内部の int64 wrap 防御）');
throws(fn() => $idx->reconstruct(-1), 'reconstruct: 負の key は例外（下側境界）');
throws(fn() => $idx->reconstruct(0, []), 'reconstruct: 旧 2 引数呼び出しは ArgumentCountError',
    ArgumentCountError::class);

finish();
