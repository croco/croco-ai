<?php

/**
 * php -d extension=build/croco_faiss.so sample.php
 */

$index = new Croco\faiss(128, 'IDMap,Flat');
echo Croco\faiss\METRIC_L2;
echo "\n";