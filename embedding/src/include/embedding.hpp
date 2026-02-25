#pragma once

#include <cmath>
#include <cstring>
#include <vector>
#include <iostream>

#include <llama.h>

namespace croco {

/**
 * Embedding class
 *
 * @package     Embedding
 * @author      Yujiro Takahashi <yujiro@cro-co.co.jp>
 */
class Embedding {
private:
    llama_model *_model;
    llama_context *_ctx;
    int32_t _nTokens;

public:
    Embedding() : _model(nullptr), _ctx(nullptr), _nTokens(0) {}
    ~Embedding() {
        if (_ctx) {
            llama_free(_ctx);
        }
        if (_model) {
            llama_model_free(_model);
        }
    }
    void loadModel(const char *model, const int32_t nThreads, uint32_t nUBatch);    
    std::vector<float> decode(const std::string text);
    std::vector<std::vector<float>> decodeList(const std::vector<std::string> &texts);

private:
    std::vector<llama_token> _tokenize(const std::string &text);
    llama_batch _toBatches(const std::vector<llama_token> &tokens);
    std::vector<float> _batchDecode(llama_batch &batch);
}; // class Embedding

/**
 * モデルの読み込み
 *
 * @access public
 * @param  const char *model モデルファイルパス
 * @param  const int32_t nThreads スレッド数
 * @param  uint32_t nUBatch 物理バッチサイズ
 * @return void
 */
inline void Embedding::loadModel(
    const char *modelPath, 
    const int32_t nThreads=4, 
    uint32_t nTokens=2048
) {
    if (_ctx) {
        llama_free(_ctx);
        _ctx = nullptr;
    }
    if (_model) {
        llama_model_free(_model);
        _model = nullptr;
    }
    _nTokens = nTokens;

    // モデルのパラメーターを設定
    llama_model_params model_params = llama_model_default_params();
    
    // モデルを読み込み
    _model = llama_load_model_from_file(modelPath, model_params);
    if (_model == NULL) {
        std::cerr << "Failed to load model from: " << modelPath << std::endl;
        return;
    }
    
    // コンテキストのパラメーターを設定
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 512;
    ctx_params.n_batch = 512;
    ctx_params.embeddings = true;  // embeddingモードを有効化
    ctx_params.n_threads = nThreads; // スレッド数を指定
    ctx_params.n_threads_batch = nThreads; // バッチ処理のスレッド
    
    // コンテキストを初期化
    _ctx = llama_new_context_with_model(_model, ctx_params);
    if (_ctx == NULL) {
        std::cerr << "Failed to create context" << std::endl;
        llama_free_model(_model);
        _model = nullptr;
        return;
    }
}

/**
 * 指定文字列の埋め込みベクトルの取得
 *
 * @access public
 * @param  const std::vector<std::string> &texts テキスト配列
 * @return std::vector<std::vector<float>> 埋め込みベクトル配列
 */
inline std::vector<float> Embedding::decode(
    const std::string text
) {
    std::vector<llama_token> tokens = _tokenize(text);

    // トークンをバッチに変換
    llama_batch batch = _toBatches(tokens);

    // バッチをデコードしてembeddingを取得
    std::vector<float> embeddings = _batchDecode(batch);

    llama_batch_free(batch);
    return embeddings;
}

/**
 * テキストリストの埋め込みベクトルを取得
 *
 * @access public
 * @param  const std::vector<std::string> &texts テキスト配列
 * @return std::vector<std::vector<float>> 埋め込みベクトル配列
 */
inline std::vector<std::vector<float>> Embedding::decodeList(
    const std::vector<std::string> &texts
) {
    std::vector<std::vector<float>> results;
    results.reserve(texts.size());
    
    for (const auto &text : texts) {
        std::vector<float> embeddings = decode(text);
        results.push_back(embeddings);
    }
    
    return results;
}

/**
 * トークン化
 *
 * @access private
 * @param  const std::string &text テキスト
 * @return std::vector<llama_token> トークン配列
 */
inline std::vector<llama_token> Embedding::_tokenize(
    const std::string &text
) {
    std::vector<llama_token> tokens;
    constexpr auto add_special = true;
    constexpr auto parse_special = false;

    // 初期容量を予測
    int n_tokens = text.length() + 2 * add_special;
    if (tokens.capacity() < static_cast<size_t>(n_tokens)) {
        tokens.reserve(n_tokens);
    }
    
    const llama_vocab *vocab = llama_model_get_vocab(_model);
    n_tokens = llama_tokenize(
        vocab,
        text.data(),
        text.length(),
        tokens.data(),
        tokens.size(),
        add_special,
        parse_special
    );
    
    if (n_tokens < 0) {
        // 推測サイズが不足している場合は実際のサイズを使用
        tokens.resize(-n_tokens);
        llama_tokenize(
            vocab,
            text.data(),
            text.length(),
            tokens.data(),
            tokens.size(),
            add_special,
            parse_special
        );
    } else {
        tokens.resize(n_tokens);
    }
    
    return tokens;
}

/**
 * トークンをバッチに変換する
 *
 * @access private
 * @param  const std::vector<llama_token> &tokens トークン配列
 * @return llama_batch バッチ
 */
llama_batch Embedding::_toBatches(
    const std::vector<llama_token> &tokens
) {
    llama_batch batch = llama_batch_init(_nTokens, 0, 1);
    const llama_seq_id sequence_id = 0;
    const auto n_tokens = static_cast<int32_t>(tokens.size());
    
    memcpy(batch.token, tokens.data(), n_tokens * sizeof(llama_token));
    
    for (int32_t idx = 0; idx < n_tokens; ++idx) {
        batch.pos[idx] = idx;
        batch.n_seq_id[idx] = 1;
        batch.seq_id[idx][0] = sequence_id;
        batch.logits[idx] = (idx == n_tokens - 1); // 最後のトークンのみ出力
    }
    
    batch.n_tokens = n_tokens;
    return batch;
}

/**
 * バッチをデコードして埋め込みベクトルを取得
 *
 * @access private
 * @param  llama_batch &batch バッチ
 * @param  int n_embd 埋め込み次元数
 * @return std::vector<float> 埋め込みベクトル
 */
std::vector<float> Embedding::_batchDecode(
    llama_batch &batch
) {
    int n_embd = llama_n_embd(_model);

    // 以前のKVキャッシュをクリア（embeddingには不要）
    llama_memory_clear(llama_get_memory(_ctx), true);

    // モデルを実行
    if (llama_decode(_ctx, batch) < 0) {
        std::cerr << "Failed to decode batch" << std::endl;
        return std::vector<float>();
    }

    // 埋め込みベクトルを取得
    std::vector<float> embeddings(n_embd, 0);
    const float *embd = llama_get_embeddings_seq(_ctx, 0);
    
    if (embd == NULL) {
        return std::vector<float>();
    }

    // embeddingをコピー
    memcpy(embeddings.data(), embd, n_embd * sizeof(float));

    // L2正規化（オプション）
    double sum = 0.0;
    for (int i = 0; i < n_embd; i++) {
        sum += embeddings[i] * embeddings[i];
    }
    
    double norm = sqrt(sum);
    if (norm > 0.0) {
        for (int i = 0; i < n_embd; i++) {
            embeddings[i] /= norm;
        }
    }

    return embeddings;
}

} // namespace croco