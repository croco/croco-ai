# faiss-php

faiss-php is a PHP bindings for faiss.

[faiss](https://github.com/facebookresearch/faiss) A library for efficient similarity search and clustering of dense vectors.

-----

## Requirements

libfaiss

```
cd /opt
curl -LO https://github.com/facebookresearch/faiss/archive/refs/tags/v1.13.2.tar.gz
tar -xzf v1.13.2.tar.gz
cd faiss-1.13.2
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DFAISS_ENABLE_GPU=OFF \
    -DFAISS_ENABLE_CUVS=OFF \
    -DFAISS_ENABLE_PYTHON=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DFAISS_ENABLE_C_API=OFF \
    -DFAISS_OPT_LEVEL=generic
cd build
make -j $(nproc)
make install
```

-----

## Building faiss for PHP

```
docker compose build
docker compose up -d
docker compose exec shell ash
```

edit your php.ini and add:

```
echo "extension=croco_faiss" > /etc/php84/conf.d/20_faiss.ini
```

-----

## Class synopsis

```php
Croco\faiss {
    const METRIC_INNER_PRODUCT = 0;
    const METRIC_L2 = 1;
    const METRIC_L1 = 2;
    const METRIC_Linf = 3;
    const METRIC_Lp = 4;
    const METRIC_Canberra = 20;
    const METRIC_BrayCurtis = 21;
    const METRIC_JensenShannon = 22;
    const METRIC_Jaccard = 23;
    const METRIC_NaNEuclidean = 24;
    const METRIC_GOWER = 25;

    public __construct(int dimension[, string description, int metric])
    public bool isTrained(voiod)
    public bool add(array vectors[, int number])
    public bool addWithIds(array vectors, array ids[, int number])
    public int ntotal(void)
    public array search(array query[, int k, int format, int number])
    public bool reset(void)
    public bool reconstruct(int key, array recons)
    public bool writeIndex(string filename)
    public bool readIndex(string filename)
    public bool importIndex(string data)
    public mixed{string|bool} exportIndex(void)
}
```
-----

## Table of Contents
* [Croco::faiss::__construct](#__construct)
* [Croco::faiss::isTrained](#istrained)
* [Croco::faiss::add](#add)
* [Croco::faiss::addWithIds](#addwithids)
* [Croco::faiss::ntotal](#ntotal)
* [Croco::faiss::search](#search)
* [Croco::faiss::reset](#reset)
* [Croco::faiss::reconstruct](#reconstruct)
* [Croco::faiss::writeIndex](#writeindex)
* [Croco::faiss::readIndex](#readindex)
* [Croco::faiss::importIndex](#importindex)
* [Croco::faiss::exportIndex](#exportindex)

-----

### <a name="__construct">Croco::faiss::__construct(int dimension[, string description, int metric])

Instantiates a faiss object.

```php
$index = new Croco\faiss(128, 'IDMap,Flat');

$index = new Croco\faiss(100, 'Flat', Croco::faiss::METRIC_L2);
```

-----

### <a name="istrained">bool Croco::faiss::isTrained(void)

Getter for is_trained.

```php
$index = new Croco\faiss(128, 'IDMap,Flat');
$res = $index->isTrained();
var_dump($res);
```

-----

### <a name="add">bool Croco::faiss::add(array data[, int number])

Add n vectors of dimension d to the index.

```php
$index = new Croco\faiss(100, 'Flat');
$vectors = [
    0.0200351,0.0941662,0.0324461,0.0755379, ......
    -0.0134401,0.00689783,0.0361747,-0.0180336, ......
    0.0744146,0.0417511,-0.0769202,0.0227152, ......
                    :
                    :
                    :
];

$object_number = 12;  // count($vectors) / dimension
$index->add($vectors, $object_number);
```

-----


### <a name="addwithids">bool Croco::faiss::addWithIds(array vectors, array ids[, int number])

Same as add, but stores xids instead of sequential ids.

```php
$index = new Croco\faiss(100, 'IDMap,Flat');
$vectors = [
    [
        0.0200351,0.0941662,0.0324461,0.0755379, ......
        -0.0134401,0.00689783,0.0361747,-0.0180336, ......
        0.0744146,0.0417511,-0.0769202,0.0227152, ......
    ],
                    :
                    :
                    :
];
$ids = [
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
];

$index->addWithIds($vectors, $ids);
```

-----

### <a name="ntotal">int Croco::faiss::ntotal()

Getter for ntotal

```php
$index = new Croco\faiss(100, 'Flat');
$index->loadIndex('sample.idx');

echo $index->ntotal();
```

-----

### <a name="search">array Croco::faiss::search(array query[, int k, int number])

```php
$index = new Croco\faiss(100, 'IDMap,Flat');
$index->loadIndex('sample.idx');

$query = [
    [
        0.0744146,0.0417511,-0.0769202,0.0227152, ......
        0.0134917,0.00398968,-0.0516475,0.0694875, ......
        -0.0531141,0.0319203,0.0229972,-0.0412282, ......
    ],
                    :
                    :
                    :
];


$res = $index->search($query, 3);
print_r($res);
```

```
[
    [0] => [
        [Rank] => 1
        [ID] => 3
        [Count] => 1
        [Distance] => 0
    ],
    [1] => [
        [Rank] => 2
        [ID] => 7
        [Count] => 1
        [Distance] => 0.023740146309137
    ],
    [2] => [
        [Rank] => 3
        [ID] => 4
        [Count] => 1
        [Distance] => 0.02716763317585
    ]
]
```

-----

### <a name="reset">bool Croco::faiss::reset()

removes all elements from the database.

```php
$index = new Croco\faiss(100, 'Flat');
$index->loadIndex('sample.idx');

$index->reset();
echo $index->ntotal();
```
-----


### <a name="reconstruct">bool Croco::faiss::reconstruct(int key, array recons)

Reconstruct a stored vector (or an approximation if lossy coding).

```php
$index = new Croco\faiss(100, 'Flat');
$index->loadIndex('sample.idx');

$recons = [
    0.00097321,0.0134312,-0.0629659,0.0388441, ......
];

$index->reconstruct(3, $recons);
```

-----

### <a name="writeindex">bool Croco::faiss::writeIndex(string filename)

Write index to a file.

```php
$index = new Croco\faiss(100, 'Flat');
$vectors = [
    0.0200351,0.0941662,0.0324461,0.0755379, ......
    -0.0134401,0.00689783,0.0361747,-0.0180336, ......
    0.0744146,0.0417511,-0.0769202,0.0227152, ......
                    :
                    :
                    :
];

$object_number = 12;  // count($vectors) / dimension

$index->add($vectors, $object_number);

$index->writeIndex('index');
```

-----

### <a name="readindex">bool Croco::faiss::readIndex(string filename)

Read index from a file.

```php
$index = new Croco\faiss(100, 'Flat');
$index->readIndex('index');

$query = [
    0.0744146,0.0417511,-0.0769202,0.0227152, ......
    0.0134917,0.00398968,-0.0516475,0.0694875, ......
    -0.0531141,0.0319203,0.0229972,-0.0412282, ......
                    :
                    :
                    :
];

$res = $index->search($query, 5, Croco\faiss\FORMAT_PLAIN, 1);
```

-----


### <a name="importindex">bool Croco::faiss::importIndex(string indexdata)

Import index.


```php
$db = new \PDO(......);
$stmt = $db->prepare('SELECT `index` FROM `faiss` WHERE `id` = :id');
$stmt->bindValue(':id', 5, \PDO::PARAM_INT);
$stmt->execute();

$data = $stmt->fetchColumn();

$index = new Croco\faiss(100, 'IDMap');
$index->importIndex($data);

$query = [
    [
        0.0744146,0.0417511,-0.0769202,0.0227152, ......
        0.0134917,0.00398968,-0.0516475,0.0694875, ......
        -0.0531141,0.0319203,0.0229972,-0.0412282, ......
    ],
                    :
                    :
                    :
];

$res = $index->search($query, 5);

```

-----


### <a name="exportindex">mixed Croco::faiss::exportIndex()

Export index.


```php
$db = new \PDO(......);

$index = new Croco\faiss(100, 'Flat');
$vectors = [
    [
        0.0200351,0.0941662,0.0324461,0.0755379, ......
        -0.0134401,0.00689783,0.0361747,-0.0180336, ......
        0.0744146,0.0417511,-0.0769202,0.0227152, ......
    ],
                    :
                    :
                    :
];
$index->add($vectors);


$stmt = $db->prepare('INSERT INTO `faiss` (`index`)VALUES(:index)');
$stmt->bindValue(':index', $index->exportIndex(), \PDO::PARAM_LOB);
$stmt->execute();
```

-----

#### Create Table
```sql
CREATE TABLE `faiss` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `index` mediumblob NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=ascii COLLATE=ascii_bin
```

License
----------
Copyright &copy; 2026 Yujiro Takahashi  
Licensed under the [MIT License][MIT].  
Distributed under the [MIT License][MIT].  

[MIT]: http://www.opensource.org/licenses/mit-license.php