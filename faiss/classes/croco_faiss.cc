#include <iostream>
#include "croco_faiss.h"
#include "croco_sort.hpp"

/* {{{ proto void faiss::__construct(int dimension[, string description, int metric])
 */
PHP_METHOD(croco_faiss_class, __construct)
{
    zend_long dimension = 768;
    char *description;
    size_t descriptionLen = 0;
    zend_long metric = faiss::METRIC_L2;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_LONG(dimension)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING(description, descriptionLen)
        Z_PARAM_LONG(metric)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        if (descriptionLen == 0) {
            description = const_cast<char *>("Flat");
        }
        faiss::Index* objIdx = faiss::index_factory(dimension, description, static_cast<faiss::MetricType>(metric), true);
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiObj->handle = reinterpret_cast<FaissHandle>(objIdx);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        return;
    }
}
/* }}} */

/* {{{ proto void faiss::__destruct()
 */
PHP_METHOD(croco_faiss_class, __destruct)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }
    php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
    faiss::Index *objIdx = reinterpret_cast<faiss::Index*>(faiObj->handle);

    delete objIdx;
}
/* }}} */

/* {{{ proto boolean faiss::isTrained()
 */
PHP_METHOD(croco_faiss_class, isTrained)
{
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        RETURN_BOOL(reinterpret_cast<const faiss::Index*>(faiObj->handle)->is_trained);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool faiss::add(array vectors[, int number])
 */
PHP_METHOD(croco_faiss_class, add)
{
    zval *array;
    zend_long number = 0;
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ARRAY(array)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(number)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        HashTable *ht = Z_ARRVAL_P(array);
        zend_hash_internal_pointer_reset(ht);
        zend_ulong size = zend_hash_num_elements(ht);
        std::vector<float> vectors;
        for (zend_ulong idx=0; idx<size; idx++) {
            zval *node = zend_hash_get_current_data(ht);
            vectors.push_back(Z_DVAL_P(node));
            zend_hash_move_forward(ht);
        } // for (zend_ulong idx=0; idx<size; idx++)

        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiss::Index *objIdx = reinterpret_cast<faiss::Index*>(faiObj->handle);
        if (number == 0) {
            number = size / objIdx->d;
        }
        objIdx->add(number, vectors.data());
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool faiss::addWithIds(array vectors, array ids[, int number])
 */
PHP_METHOD(croco_faiss_class, addWithIds)
{
    zval *array;
    zval *idArray;
    zend_long number = 0;
    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_ARRAY(array)
        Z_PARAM_ARRAY(idArray)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(number)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        HashTable *ht = Z_ARRVAL_P(array);
        zend_hash_internal_pointer_reset(ht);
        zend_ulong size = zend_hash_num_elements(ht);
        std::vector<float> vectors;
        for (zend_ulong idx=0; idx<size; idx++) {
            zval *node = zend_hash_get_current_data(ht);
            vectors.push_back(Z_DVAL_P(node));
            zend_hash_move_forward(ht);
        } // for (zend_ulong idx=0; idx<size; idx++)

        HashTable *idHt = Z_ARRVAL_P(idArray);
        zend_hash_internal_pointer_reset(idHt);
        zend_ulong idSize = zend_hash_num_elements(idHt);
        std::vector<faiss::idx_t> ids;
        for (zend_ulong idx=0; idx<idSize; idx++) {
            zval *node = zend_hash_get_current_data(idHt);
            ids.push_back(Z_LVAL_P(node));
            zend_hash_move_forward(idHt);
        } // for (zend_ulong idx=0; idx<size; idx++)

        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiss::Index *objIdx = reinterpret_cast<faiss::Index*>(faiObj->handle);
        if (number == 0) {
            number = size / objIdx->d;
        }
        objIdx->add_with_ids(number, vectors.data(), ids.data());
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto int faiss::ntotal()
 */
PHP_METHOD(croco_faiss_class, ntotal)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }
    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        RETURN_LONG(reinterpret_cast<const faiss::Index*>(faiObj->handle)->ntotal);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_LONG(0);
    }
}
/* }}} */

/* {{{ proto array faiss::search(array query[, int k, int format, int number])
 */
PHP_METHOD(croco_faiss_class, search)
{
    zval *array;
    zend_long k = 0;
    zend_long format = 0;
    zend_long number = 0;
    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_ARRAY(array)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(k)
        Z_PARAM_LONG(format)
        Z_PARAM_LONG(number)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        HashTable *ht = Z_ARRVAL_P(array);
        zend_hash_internal_pointer_reset(ht);
        zend_ulong size = zend_hash_num_elements(ht);
        std::vector<float> querys;
        for (zend_ulong idx=0; idx<size; idx++) {
            zval *node = zend_hash_get_current_data(ht);
            querys.push_back(Z_DVAL_P(node));
            zend_hash_move_forward(ht);
        } // for (zend_ulong idx=0; idx<size; idx++)

        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        const faiss::Index *objIdx = reinterpret_cast<const faiss::Index*>(faiObj->handle);
        if (0 == k) {
            float x = std::sqrt(objIdx->ntotal);
            k = static_cast<zend_long>(x + 0.5f);
        }
        if (0 == number) {
            number = size / objIdx->d;
        }

        faiss::idx_t n = number;
        float distances[k];
        faiss::idx_t labels[k];

        objIdx->search(n, querys.data(), k, distances, labels);

        std::vector<croco::stats_t> stats = croco::FaissStatsFormat(distances, labels, k);
        zend_long idx = 0;
        array_init(return_value);
        for (const auto& stat : stats) {
            zval rowVal, rankVal, countVal, distVal, labelVal;

            ZVAL_LONG(&rankVal, idx + 1);
            ZVAL_LONG(&labelVal, stat.id);
            ZVAL_LONG(&countVal, stat.count);
            ZVAL_DOUBLE(&distVal, stat.distance);

            array_init(&rowVal);
            zend_hash_str_add(Z_ARRVAL_P(&rowVal), "Rank", sizeof("Rank")-1, &rankVal);
            zend_hash_str_add(Z_ARRVAL_P(&rowVal), "ID", sizeof("ID")-1, &labelVal);
            zend_hash_str_add(Z_ARRVAL_P(&rowVal), "Count", sizeof("Count")-1, &countVal);
            zend_hash_str_add(Z_ARRVAL_P(&rowVal), "Distance", sizeof("Distance")-1, &distVal);

            add_index_zval(return_value, idx, &rowVal);
            idx++;
        }
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto bool faiss::reset()
 */
PHP_METHOD(croco_faiss_class, reset)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }
    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        reinterpret_cast<faiss::Index*>(faiObj->handle)->reset();
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool faiss::reconstruct(int key, array recons)
 */
PHP_METHOD(croco_faiss_class, reconstruct)
{
    zend_long key;
    zval *array;
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(key)
        Z_PARAM_ARRAY(array)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        HashTable *ht = Z_ARRVAL_P(array);
        zend_hash_internal_pointer_reset(ht);
        zend_ulong size = zend_hash_num_elements(ht);
        std::vector<float> recons;
        for (zend_ulong idx=0; idx<size; idx++) {
            zval *node = zend_hash_get_current_data(ht);
            recons.push_back(Z_DVAL_P(node));
            zend_hash_move_forward(ht);
        } // for (zend_ulong idx=0; idx<size; idx++)

        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        reinterpret_cast<faiss::Index*>(faiObj->handle)->reconstruct(key, recons.data());
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool faiss::writeIndex(string filename)
 */
PHP_METHOD(croco_faiss_class, writeIndex)
{
    char *file;
    size_t file_len = 0;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(file, file_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        const faiss::Index *objIdx = reinterpret_cast<const faiss::Index*>(faiObj->handle);
        faiss::write_index(objIdx, file);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool faiss::readIndex(string filename)
 */
PHP_METHOD(croco_faiss_class, readIndex)
{
    char *file;
    size_t file_len = 0;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(file, file_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        auto out = faiss::read_index(file, faiss::IO_FLAG_ONDISK_SAME_DIR);

        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiObj->handle = reinterpret_cast<FaissHandle>(out);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool faiss::importIndex(string data)
 */
PHP_METHOD(croco_faiss_class, importIndex)
{
    char *data;
    size_t data_len = 0;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(data, data_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        FILE *fp = fmemopen((void*)data, data_len, "rb");
        auto out = faiss::read_index(fp, faiss::IO_FLAG_MMAP);

        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiObj->handle = reinterpret_cast<FaissHandle>(out);

        fclose(fp);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string faiss::exportIndex()
 */
PHP_METHOD(croco_faiss_class, exportIndex)
{
    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        const faiss::Index *objIdx = reinterpret_cast<const faiss::Index*>(faiObj->handle);

        char *data;
        size_t size = 0;
        FILE *fp = open_memstream(&data, &size);
        faiss::write_index(objIdx, fp);

        fflush(fp);
        ZVAL_STRINGL(return_value, data, size);

        fclose(fp);
        free(data);
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
}
/* }}} */
