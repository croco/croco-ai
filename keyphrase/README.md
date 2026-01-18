Keyphrase
======================
pagerank using faiss.

---

## 目次

* [faissのインストール](#install)
* [Docker環境実行](#docker)
* [キーフレーズ取得](#sample00)
* [フレーズ候補取得](#sample01)


## <a name="install">faissのインストール

```
curl -LO https://github.com/facebookresearch/faiss/archive/refs/tags/v1.13.2.tar.gz \
tar -xzf v1.13.2.tar.gz \
cd faiss-1.13.2 \
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

## <a name="docker">Docker環境実行  
```
docker compose build
docker compose up -d
docker compose exec shell ash
```

## <a name="sample00">キーフレーズ取得

```php
$keyphrase = new Croco\Keyphrase('/usr/lib/mecab/dic/ipadic/');
$text = <<<TEXT
たぶん子供のころから、誰かの喜ぶ顔や笑顔を見ることが好きでした。誰かのために動いた結果、その誰かが脚光を浴びたり、褒められたり、感謝されている姿を見られることに幸せを感じました。たとえ仕掛けたのが自分だと気づかれなくても、自分自身が表に出ることはなくても。
人材ビジネスを行っている企業に新卒として入社をしたばかりの頃、その"誰か"は人材を募集しているお取引先の担当者の方々と、仕事を探している求職者の方々でした。お取引先の担当者のために、転職希望者の方のために頭と体に汗をかき続けました。対峙する方々の喜ぶ顔を見る
につれ、もっと多くの人に喜んでもらいたいという思いが募りました。1社でも多くのお取引先、1人でも多くの求職者の方に喜んでもらうための仕組みづくりをしようと会社の中にHR部門を立ち上げました。自社の社員がお客様に褒められたり認められることが喜びになりました。
採用や組織開発に従事し一定の仕組み化ができてきたら、もっと多様な喜びを多様な人に提供したいと思うようになり、事業開発も行うようになりました。その矢先にリーマンショックが訪れ、自身が身を置いていた「場所」「時間」にとらわれる業界構造にやるせなさを感じる日々が訪れました。
そんな折、縁あってインターネットサービスを提供している会社の方との接点を持つ機会があり、インターネットの可能性に魅了されました。時間も場所も距離もコストも取っ払い、人と人、人と作品、人と企業、人とデータ、人とモノ、企業と企業、企業とデータetc...を
あらゆる点をつなげることができれば、指数関数的に"誰か"の笑顔を生み出し続けることができるではないか、と。
TEXT;

$keyphrases = $keyphrase->extract($text);
foreach ($keyphrases as $row) {
    echo $row['phrase']."\t\t".$row['weight'];
    echo "\n";
}
```

### 出力結果 ###

```
インターネットサービス		0.11566668003798
お客様		0.093158096075058
ライター		0.055224109441042
コンテンツ		0.046349070966244
インターネット		0.042762130498886
                ：
                ：
                ：
```

### 利用目的

フレーズ候補をもとに本文全体とフレーズの近傍検索によりフレーズの順位付などに利用できます。

ライセンス
----------
Copyright &copy; 2025 Yujiro Takahashi  
Licensed under the [MIT License][MIT].  
Distributed under the [MIT License][MIT].  

[MIT]: http://www.opensource.org/licenses/mit-license.php