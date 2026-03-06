#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <mutex>

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
    std::mutex _mutex;
    uint32_t _nBatch;
    uint32_t _nUBatch;

public:
    Embedding() : _model(nullptr), _ctx(nullptr) {}
    ~Embedding() {
        if (_ctx) {
            llama_free(_ctx);
        }
        if (_model) {
            llama_model_free(_model);
        }
    }
    void loadModel(const char *model, const int32_t nThreads, uint32_t nUBatch);    
    std::vector<std::vector<float>> decodeList(const std::vector<std::string> &texts);
    const uint32_t getBatchSize() { return _nBatch; }
    const uint32_t getUBatchSize() { return _nUBatch; }

private:
    std::vector<llama_token> _tokenize(const std::string &text);
    void _addBatch(llama_batch &batch, llama_token id, llama_pos pos, const std::vector<llama_seq_id> &seqIds, bool logits);
    std::vector<float> _getEmbedding(const llama_seq_id sequence_id);
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
inline void Embedding::loadModel(const char *model, const int32_t nThreads=4, uint32_t nUBatch=2048)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_ctx) {
        llama_free(_ctx);
        _ctx = nullptr;
    }

    if (_model) {
        llama_model_free(_model);
        _model = nullptr;
    }

    // モデルパラメータの設定
    llama_model_params model_params = llama_model_default_params();
    
    // モデルの読み込み
    _model = llama_model_load_from_file(model, model_params);
    if (!_model) {
        std::cerr << "Failed to load model" << std::endl;
        return;
    }

    // コンテキストパラメータの設定
    llama_context_params ctx_params = llama_context_default_params();
    // コンテキストサイズは物理バッチサイズの4倍程度に設定
    ctx_params.n_ctx = nUBatch * 4;             // 大きなコンテキストサイズ
    ctx_params.n_batch = nUBatch;               // logical batch size
    ctx_params.n_ubatch = nUBatch;              // physical maximum batch size
    ctx_params.n_seq_max = 256;                 // 最大シーケンス数を増やす
    ctx_params.n_threads = nThreads;            // スレッド数を設定
    ctx_params.n_threads_batch = nThreads;      // バッチ処理スレッド数
    ctx_params.pooling_type = LLAMA_POOLING_TYPE_MEAN;  // pooling: mean
    ctx_params.embeddings = true;               // embeddings を有効化

    _nBatch = ctx_params.n_batch;
    _nUBatch = ctx_params.n_ubatch;
    
    // コンテキストの初期化
    _ctx = llama_init_from_model(_model, ctx_params);
    
    if (!_ctx) {
        std::cerr << "Failed to create context" << std::endl;
        llama_model_free(_model);
        return ;
    }
}

/**
 * 指定文字列の埋め込みベクトルの取得
 *
 * @access public
 * @param  const std::vector<std::string> &texts テキスト配列
 * @return std::vector<std::vector<float>> 埋め込みベクトル配列
 */
inline std::vector<std::vector<float>> Embedding::decodeList(const std::vector<std::string> &texts)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // 結果配列
    std::vector<std::vector<float>> embeddings;

    if (texts.empty()) {
        return embeddings;
    }

    if (!_ctx || !_model) {
        std::cerr << "Model is not loaded" << std::endl;
        return embeddings;
    }

    std::vector<std::vector<llama_token>> tokenList;
    tokenList.reserve(texts.size());

    size_t total_tokens = 0;
    for (const auto &text : texts) {
        auto tokens = _tokenize(text);
        if (tokens.empty()) {
            continue;
        }
        total_tokens += tokens.size();
        tokenList.emplace_back(std::move(tokens));
    }

    if (tokenList.empty()) {
        std::cerr << "Failed to tokenize input texts" << std::endl;
        return embeddings;
    }
        
    // 複数バッチに分割して処理
    embeddings.reserve(tokenList.size());
    
    // メモリクリア（最初に一度だけ）
    llama_memory_clear(llama_get_memory(_ctx), true);
    
    // バッチを分割して処理（複数回のデコード呼び出し）
    size_t current_idx = 0;
    while (current_idx < tokenList.size()) {
        // 現在のバッチに含められるシーケンス数を計算
        size_t batch_tokens = 0;
        size_t batch_end = current_idx;
        
        for (size_t i = current_idx; i < tokenList.size(); ++i) {
            size_t next_tokens = batch_tokens + tokenList[i].size();
            if (next_tokens > static_cast<size_t>(_nUBatch) && batch_end > current_idx) {
                // バッチサイズを超える場合、ここで区切る（ただし最低1つは含める）
                break;
            }
            batch_tokens += tokenList[i].size();
            batch_end = i + 1;
            
            // バッチサイズに達したら区切る
            if (batch_tokens >= static_cast<size_t>(_nUBatch)) {
                break;
            }
        }

        // バッチの作成
        llama_batch batch = llama_batch_init(
            static_cast<int32_t>(batch_tokens), 
            0, 
            static_cast<int32_t>(batch_end - current_idx)
        );
        batch.n_tokens = 0;

        // バッチに全トークンを追加
        for (size_t seq_idx = current_idx; seq_idx < batch_end; ++seq_idx) {
            llama_pos pos = 0;
            llama_seq_id local_seq_id = static_cast<llama_seq_id>(seq_idx - current_idx);
            
            for (size_t token_idx = 0; token_idx < tokenList[seq_idx].size(); ++token_idx) {
                llama_token token = tokenList[seq_idx][token_idx];
                bool is_last = (token_idx == tokenList[seq_idx].size() - 1);
                
                _addBatch(batch, token, pos++, { local_seq_id }, is_last);
            }
        }

        if (batch.n_tokens == 0) {
            std::cerr << "Warning: Empty batch at index " << current_idx << std::endl;
            llama_batch_free(batch);
            current_idx = batch_end;
            continue;
        }

        // バッチを処理
        const int ret = llama_decode(_ctx, batch);
        if (ret != 0) {
            std::cerr << "Failed to decode batch: " << ret << std::endl;
            std::cerr << "Return code meanings: 1=no KV slot, 2=aborted, -1=invalid input, <-1=fatal error" << std::endl;
            std::cerr << "Batch info: " << batch.n_tokens << " tokens, " 
                      << (batch_end - current_idx) << " sequences" << std::endl;
            llama_batch_free(batch);
            return embeddings;
        }

        // このバッチの埋め込みを取得
        for (size_t seq_idx = current_idx; seq_idx < batch_end; ++seq_idx) {
            llama_seq_id local_seq_id = static_cast<llama_seq_id>(seq_idx - current_idx);
            auto embedding = _getEmbedding(local_seq_id);
            
            if (embedding.empty()) {
                std::cerr << "Warning: Empty embedding for sequence " << seq_idx << std::endl;
            }
            
            embeddings.emplace_back(std::move(embedding));
        }

        llama_batch_free(batch);
        current_idx = batch_end;
    }

    return embeddings;
}


/**
 * トークン化
 *
 * @access private
 * @param  const std::string &text テキスト
 * @return std::vector<llama_token> トークン配列
 */
inline std::vector<llama_token> Embedding::_tokenize(const std::string &text)
{
    // トークン化
    std::vector<llama_token> tokens;
    constexpr auto add_special = true;
    constexpr auto parse_special = false;

    int n_tokens = text.length() + 2 * add_special;
    tokens.resize(n_tokens);
    
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
 * バッチにトークンを追加
 *
 * @access private
 * @param  llama_batch &batch バッチ
 * @param  llama_token id トークンID
 * @param  llama_pos pos 位置
 * @param  const std::vector<llama_seq_id> &seq_ids シーケンスID配列
 * @param  bool logits ロジットフラグ
 * @return void
 */
inline void Embedding::_addBatch(
    llama_batch &batch, 
    llama_token id, 
    llama_pos pos, 
    const std::vector<llama_seq_id> &seqIds, 
    bool logits
){
    batch.token   [batch.n_tokens] = id;
    batch.pos     [batch.n_tokens] = pos;
    batch.n_seq_id[batch.n_tokens] = seqIds.size();
    for (size_t i = 0; i < seqIds.size(); ++i) {
        batch.seq_id[batch.n_tokens][i] = seqIds[i];
    }
    batch.logits  [batch.n_tokens] = logits;

    batch.n_tokens++;
}

/**
 * 埋め込みベクトルの取得
 *
 * @access private
 * @param  const std::string &text テキスト
 * @return std::vector<float> 埋め込みベクトル
 */
inline std::vector<float> Embedding::_getEmbedding(const llama_seq_id sequence_id)
{
    // Embeddingを取得
    const int n_embd = llama_model_n_embd(_model);
    const float *raw_embedding = llama_get_embeddings_seq(_ctx, sequence_id);
    
    if (!raw_embedding) {
        std::cerr << "Failed to get embeddings" << std::endl;
        return std::vector<float>();
    }

    // 正規化（L2ノルム）
    std::vector<float> normalized_embedding(n_embd);
    float square_sum = 0.0f;
    for (int i = 0; i < n_embd; ++i) {
        square_sum += raw_embedding[i] * raw_embedding[i];
    }
    
    const float magnitude = std::sqrt(square_sum);
    const float normalize = magnitude > 0.0f ? 1.0f / magnitude : 0.0f;
    
    for (int i = 0; i < n_embd; ++i) {
        normalized_embedding[i] = raw_embedding[i] * normalize;
    }

    return normalized_embedding;
}

} // namespace croco