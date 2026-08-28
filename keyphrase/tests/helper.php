<?php
// 実機ビルドした croco_keyphrase 拡張の受け入れテスト用ヘルパ
// 実行例: php -d extension=/path/to/croco_keyphrase.so tests/test_keyphrase.php

$GLOBALS['__test_failures'] = 0;

function ok(bool $cond, string $name): void
{
    if ($cond) {
        echo "ok - {$name}\n";
    } else {
        $GLOBALS['__test_failures']++;
        echo "FAIL - {$name}\n";
    }
}

/**
 * ipadic の場所。環境で変わるので MECAB_DIC_PATH で差し替えられるようにする
 */
function dicPath(): string
{
    return getenv('MECAB_DIC_PATH') ?: '/usr/lib/mecab/dic/ipadic';
}

function requireKeyphrase(): void
{
    if (!class_exists('Croco\Keyphrase')) {
        fwrite(STDERR, "SKIP: croco_keyphrase 拡張がロードされていません（php -d extension=... で実行してください）\n");
        exit(77); // テストハーネス慣習の SKIP コード
    }
    if (!is_dir(dicPath())) {
        fwrite(STDERR, "SKIP: mecab 辞書が見つかりません: " . dicPath() . "（MECAB_DIC_PATH で指定できます）\n");
        exit(77);
    }
}

function finish(): void
{
    $n = $GLOBALS['__test_failures'];
    echo $n === 0 ? "all tests passed\n" : "{$n} test(s) failed\n";
    exit($n === 0 ? 0 : 1);
}
