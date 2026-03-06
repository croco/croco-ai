<?php

/**
 * php -d extension=build/croco_embedding.so sample.php
 */

$embedding = new Croco\Embedding(
    '/models/embeddinggemma-300M-GGUF/embeddinggemma-300M-Q8_0.gguf', 1, 2048
);
// 11926
$text = file_get_contents('/home/croco/embedding/large.txt');

$vecs = $embedding->decode($text);
echo "\t";
foreach ($vecs as $idx => $vec) {
    echo $vec . ", ";
    if (($idx + 1) % 5 == 0) {
        echo "\n\t";
    }
}
echo "\n\n";
