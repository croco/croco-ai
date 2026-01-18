Embedding
======================
Node.js Addons embedding using llama.cpp.

---

## 目次

* [llama.cppのインストール](#install)
* [Docker環境実行](#docker)
* [利用方法](#sample00)


## <a name="install">llama.cppのインストール

```
git clone --recurse-submodules https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_SHARED_LIBS=ON \
    -DLLAMA_BUILD_EXAMPLES=OFF \
    -DLLAMA_BUILD_TOOLS=OFF \
    -DLLAMA_BUILD_TESTS=OFF \
    -DLLAMA_BUILD_SERVER=OFF \
    -DGGML_METAL=OFF \
    -DGGML_METAL_EMBED_LIBRARY=OFF \
    -DGGML_BLAS_DEFAULT=ON \
    -DGGML_METAL_USE_BF16=ON \
    -DGGML_OPENMP=OFF \
    -DNDEBUG=ON
cd build
make -j $(nproc)
make install
```

[Embeddingモデル:GGUF形式](https://huggingface.co/ggml-org/embeddinggemma-300M-GGUF)  
llama.cppに対応したモデルを指定します  

## <a name="docker">Docker環境実行  
```
docker compose build
docker compose up -d
docker compose exec shell ash
```

## <a name="sample00">利用方法

```php

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
```

### 出力結果 ###

```
	0.010517320595682, -0.030097108334303, -0.042305465787649, -0.018959147855639, -0.040021244436502, 
	-0.0051778028719127, -0.00030565212364309, 0.012723940424621, 0.0086268428713083, 0.057487733662128, 
	-0.034802973270416, -0.026165394112468, 0.0084467502310872, -0.061711642891169, -0.028247836977243, 
	0.024144044145942, 0.033788207918406, 0.061654940247536, -0.046111565083265, 0.038330309092999, 
                                    :
                                    :
                                    :

	0.0085062924772501, -0.021689945831895, -0.07046852260828, -0.026205856353045, 0.016773292794824, 
	-0.00025846902281046, 0.010838663205504, 0.0099951913580298, 0.0086376387625933, 0.047525383532047, 
	-0.026118651032448, -0.028195636346936, 0.041378878057003, 0.018095403909683, -0.00078804738586769, 
	0.01977751031518, 0.0264199282974, 0.043091703206301, -0.041895229369402, 0.03718775510788, 
                                    :
                                    :
                                    :
```
    

ライセンス
----------
Copyright &copy; 2025 Yujiro Takahashi  
Licensed under the [MIT License][MIT].  
Distributed under the [MIT License][MIT].  

[MIT]: http://www.opensource.org/licenses/mit-license.php