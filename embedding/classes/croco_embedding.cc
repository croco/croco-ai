#include <iostream>
#include "croco_embedding.h"

/**
 * ログコールバック関数（何もしない）
 *
 * @param  enum ggml_log_level level ログレベル
 * @param  const char * text ログテキスト
 * @param  void * user_data ユーザーデータ
 * @return void
 */
static void llama_log_callback_null(ggml_log_level level, const char * text, void * user_data) {
    (void) level;
    (void) text;
    (void) user_data;
}

/* {{{ proto void embedding::__construct()
 */
PHP_METHOD(croco_embedding_class, __construct)
{
    char *modelPath;
    size_t modelPathLen = 0;
    zend_long threadNum = 4;
    zend_long batchSize = 2048;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STRING(modelPath, modelPathLen)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(threadNum)
        Z_PARAM_LONG(batchSize)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        // ログコールバック関数の設定
        llama_log_set(llama_log_callback_null, NULL);

        croco::Embedding *objEmb = new croco::Embedding();
        objEmb->loadModel(modelPath, threadNum, batchSize);

        php_croco_embedding_object *idx_obj = Z_EMBEDDING_P(ZEND_THIS);
        idx_obj->handle = reinterpret_cast<EmbeddingHandle>(objEmb);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto void embedding::__destruct()
 */
PHP_METHOD(croco_embedding_class, __destruct)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }
    php_croco_embedding_object *idx_obj = Z_EMBEDDING_P(ZEND_THIS);
    croco::Embedding *objEmb = reinterpret_cast<croco::Embedding*>(idx_obj->handle);

    delete objEmb;
}
/* }}} */

/* {{{ proto array embedding::getEmbeddings(texts: array)
 */
PHP_METHOD(croco_embedding_class, getEmbeddings)
{
    zval *array;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(array)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        HashTable *ht = Z_ARRVAL_P(array);
        zend_hash_internal_pointer_reset(ht);
        zend_ulong size = zend_hash_num_elements(ht);

        std::vector<std::string> texts;
        for (zend_ulong idx=0; idx<size; idx++) {
            zval *node = zend_hash_get_current_data(ht);
            texts.push_back(Z_STRVAL_P(node));
            zend_hash_move_forward(ht);
        }

        php_croco_embedding_object *idx_obj = Z_EMBEDDING_P(ZEND_THIS);
        std::vector<std::vector<float>> embeddings = reinterpret_cast<croco::Embedding*>(idx_obj->handle)->getEmbeddings(texts);

        array_init(return_value);
        if (embeddings.empty()) {
            return;
        }

        for (size_t i = 0; i < embeddings.size(); ++i) {
            zval rVals;
            array_init(&rVals);
            for (size_t j = 0; j < embeddings[i].size(); ++j) {
                zval val;
                ZVAL_DOUBLE(&val, embeddings[i][j]);
                add_index_zval(&rVals, j, &val);
            }
            add_index_zval(return_value, i, &rVals);
        }
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
}
/* }}} */
