<?php
// 実機ビルドした croco_faiss 拡張の受け入れテスト用ヘルパ
// 実行例: php -d extension=/path/to/croco_faiss.so tests/test_faiss.php

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

function throws(callable $fn, string $name, string $class = ErrorException::class, ?string $needle = null): void
{
    try {
        $fn();
        $GLOBALS['__test_failures']++;
        echo "FAIL - {$name}（例外が発生しなかった）\n";
    } catch (Throwable $e) {
        if (!($e instanceof $class)) {
            $GLOBALS['__test_failures']++;
            echo "FAIL - {$name}（期待 {$class}、実際 " . get_class($e) . ": {$e->getMessage()}）\n";
        } elseif ($needle !== null && !str_contains($e->getMessage(), $needle)) {
            // 例外クラスだけでは検証経路を取り違えるため、メッセージ断片でも突き合わせる
            $GLOBALS['__test_failures']++;
            echo "FAIL - {$name}（メッセージに '{$needle}' を含まない: {$e->getMessage()}）\n";
        } else {
            echo "ok - {$name}\n";
        }
    }
}

function requireFaiss(): void
{
    if (!class_exists('Croco\Faiss')) {
        fwrite(STDERR, "SKIP: croco_faiss 拡張がロードされていません（php -d extension=... で実行してください）\n");
        exit(77); // テストハーネス慣習の SKIP コード
    }
}

function finish(): void
{
    $n = $GLOBALS['__test_failures'];
    echo $n === 0 ? "all tests passed\n" : "{$n} test(s) failed\n";
    exit($n === 0 ? 0 : 1);
}
