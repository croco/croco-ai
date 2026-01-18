<?php

/**
 * php -d extension=build/croco_embedding.so sample.php
 */

$embedding = new Croco\Embedding(
    '/models/embeddinggemma-300M-GGUF/embeddinggemma-300M-Q8_0.gguf'
);
$texts = [
    "ビジネス文書の使い方「ビジネス文書」では、「招待状・案内状」や「始末書」",
    "「経緯報告書」など、社内・社外（取引先）に向けて作成する文書を生成することができます。"
];
$embeddings = $embedding->getEmbeddings($texts);
foreach ($embeddings as $vecs) {
    echo "\t";
    foreach ($vecs as $idx => $vec) {
        echo $vec . ", ";
        if (($idx + 1) % 5 == 0) {
            echo "\n\t";
        }
    }
    echo "\n\n";
}