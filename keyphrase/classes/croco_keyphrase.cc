#include <iostream>
#include "croco_keyphrase.h"

/* {{{ proto void keyphrase::__construct()
 */
PHP_METHOD(croco_keyphrase_class, __construct)
{
    char *dicDir;
    size_t dicDirLen = 0;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(dicDir, dicDirLen)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        croco::Sentence *objSent = new croco::Sentence(dicDir);
        php_croco_keyphrase_object *idx_obj = Z_KEYPHRASE_P(ZEND_THIS);
        idx_obj->handle = reinterpret_cast<KeyphraseHandle>(objSent);
    } catch (const std::exception &e) {
        zend_throw_exception(zend_exception_get_default(), e.what(), 0);
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto void keyphrase::__destruct()
 */
PHP_METHOD(croco_keyphrase_class, __destruct)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }
    php_croco_keyphrase_object *idx_obj = Z_KEYPHRASE_P(ZEND_THIS);
    croco::Sentence *objSent = reinterpret_cast<croco::Sentence*>(idx_obj->handle);

    delete objSent;
}
/* }}} */

/* {{{ proto void keyphrase::extract(string text)
 */
PHP_METHOD(croco_keyphrase_class, extract)
{
    char *text;
    size_t text_len = 0;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(text, text_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        php_croco_keyphrase_object *idx_obj = Z_KEYPHRASE_P(ZEND_THIS);
        auto sentences = reinterpret_cast<croco::Sentence*>(idx_obj->handle)->parse(text);

        croco::Lines lineParser;
        auto lines = lineParser.parse(sentences.wordLines, sentences.posLines);

        croco::Phrases phraseParser;
        auto candidates = phraseParser.parse(lines);

        croco::MultipartiteRank rank;
        auto nodes = rank.getKeyPhrase(candidates);

        array_init(return_value);
        for (size_t idx = 0; idx < nodes.size(); idx++) {
            zval rVals, rPhrase, rWeight;

            array_init(&rVals);
            ZVAL_STRING(&rPhrase, nodes[idx].phrase.c_str());
            ZVAL_DOUBLE(&rWeight, nodes[idx].weight);

            zend_hash_str_add(Z_ARRVAL(rVals), "phrase", sizeof("phrase")-1, &rPhrase);
            zend_hash_str_add(Z_ARRVAL(rVals), "weight", sizeof("weight")-1, &rWeight);
            add_index_zval(return_value, idx, &rVals);
        }
    } catch (const std::exception &e) {
        zend_throw_exception(zend_exception_get_default(), e.what(), 0);
        RETURN_FALSE;
    }
}         
/* }}} */

/* {{{ proto void keyphrase::candidate()
 */
PHP_METHOD(croco_keyphrase_class, candidate)
{
    char *text;
    size_t text_len = 0;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(text, text_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        php_croco_keyphrase_object *idx_obj = Z_KEYPHRASE_P(ZEND_THIS);
        auto sentences = reinterpret_cast<croco::Sentence*>(idx_obj->handle)->parse(text);

        croco::Lines lineParser;
        auto lines = lineParser.parse(sentences.wordLines, sentences.posLines);

        croco::Phrases phraseParser;
        auto candidates = phraseParser.parse(lines);

        array_init(return_value);
        for (size_t idx = 0; idx < candidates.keys.size(); idx++) {
            zval rPhrase;
            ZVAL_STRING(&rPhrase, candidates.keys[idx].c_str());
            add_index_zval(return_value, idx, &rPhrase);
        }
    } catch (const std::exception &e) {
        zend_throw_exception(zend_exception_get_default(), e.what(), 0);
        RETURN_FALSE;
    }
}         
/* }}} */
