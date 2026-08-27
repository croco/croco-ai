#include <iostream>
#include <cstdint>
#include <stdexcept>
#include "croco_faiss.h"
#include "croco_sort.hpp"

namespace {

/**
 * PHP 配列を float ベクトルへ変換する
 *
 * 従来は zval を無検査で double として読んでいたため、float/int 以外の要素が
 * 混ざるとビットの再解釈で値が壊れていた。型を検査し、想定外の要素は例外にする。
 */
std::vector<float> phpArrayToFloatVector(zval *array)
{
    HashTable *ht = Z_ARRVAL_P(array);
    std::vector<float> vectors;
    vectors.reserve(zend_hash_num_elements(ht));

    zval *node;
    ZEND_HASH_FOREACH_VAL(ht, node) {
        ZVAL_DEREF(node); // foreach (... as &$v) 後の配列などに残る参照を実体まで解決する
        if (Z_TYPE_P(node) == IS_DOUBLE) {
            vectors.push_back(static_cast<float>(Z_DVAL_P(node)));
        } else if (Z_TYPE_P(node) == IS_LONG) {
            vectors.push_back(static_cast<float>(Z_LVAL_P(node)));
        } else {
            throw std::invalid_argument("vector elements must be int or float");
        }
    } ZEND_HASH_FOREACH_END();

    return vectors;
}

/**
 * 配列の要素数と次元から行数を検証・決定する
 *
 * 従来は number = size / d の切り捨てで決めていたため、要素数が次元の倍数で
 * ない配列は 0 行として黙って捨てられていた（faiss の ID と呼び出し側の添字が
 * ずれる原因）。端数が出る入力・行数と合わない number 指定は例外にする。
 */
zend_long resolveRowCount(size_t size, zend_long number, zend_long dimension)
{
    if (dimension <= 0) {
        // new \Croco\faiss(0) は index_factory を通ってしまうため、ここで弾かないと
        // size % 0 の 0 除算で PHP プロセスごと落ちる
        throw std::invalid_argument("index dimension must be positive");
    }
    if (0 == size || 0 != (size % static_cast<size_t>(dimension))) {
        throw std::invalid_argument(
            "array length (" + std::to_string(size)
            + ") must be a positive multiple of dimension (" + std::to_string(dimension) + ")");
    }
    // 乗算 (number * dimension) は size_t で wrap しうるため、検証は除算結果との比較で行う
    const zend_long rows = static_cast<zend_long>(size / static_cast<size_t>(dimension));
    if (0 == number) {
        return rows;
    }
    if (number != rows) {
        throw std::invalid_argument(
            "number (" + std::to_string(number) + ") does not match array length ("
            + std::to_string(size) + ") / dimension (" + std::to_string(dimension) + ")");
    }
    return number;
}

} // namespace

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
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiss::Index *objIdx = reinterpret_cast<faiss::Index*>(faiObj->handle);

        std::vector<float> vectors = phpArrayToFloatVector(array);
        number = resolveRowCount(vectors.size(), number, objIdx->d);
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
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        faiss::Index *objIdx = reinterpret_cast<faiss::Index*>(faiObj->handle);

        std::vector<float> vectors = phpArrayToFloatVector(array);
        number = resolveRowCount(vectors.size(), number, objIdx->d);

        HashTable *idHt = Z_ARRVAL_P(idArray);
        if (static_cast<zend_long>(zend_hash_num_elements(idHt)) != number) {
            throw std::invalid_argument(
                "ids length (" + std::to_string(zend_hash_num_elements(idHt))
                + ") must equal the number of vectors (" + std::to_string(number) + ")");
        }
        std::vector<faiss::idx_t> ids;
        ids.reserve(number);
        zval *node;
        ZEND_HASH_FOREACH_VAL(idHt, node) {
            ZVAL_DEREF(node);
            if (Z_TYPE_P(node) != IS_LONG) {
                throw std::invalid_argument("ids elements must be int");
            }
            if (Z_LVAL_P(node) == -1) {
                // -1 は faiss が「候補なし」の番兵に使う予約値。登録を許すと
                // 検索結果から黙って除外され、silent drop が再発する
                throw std::invalid_argument("id -1 is reserved by faiss as the missing-result sentinel");
            }
            ids.push_back(Z_LVAL_P(node));
        } ZEND_HASH_FOREACH_END();

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
    zend_long format = 0; // 予約引数（現状未実装。出力は常に Rank/ID/Count/Distance の配列）
    zend_long number = 0;
    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_ARRAY(array)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(k)
        Z_PARAM_LONG(format)
        Z_PARAM_LONG(number)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        const faiss::Index *objIdx = reinterpret_cast<const faiss::Index*>(faiObj->handle);

        std::vector<float> querys = phpArrayToFloatVector(array);
        number = resolveRowCount(querys.size(), number, objIdx->d);

        if (k < 0) {
            throw std::invalid_argument("k must be a positive integer");
        }
        if (0 == k) {
            float x = std::sqrt(objIdx->ntotal);
            k = static_cast<zend_long>(x + 0.5f);
        }
        if (objIdx->ntotal < 0) {
            // 破損・細工されたインデックスファイル由来。クランプで k が負になる前に
            // 原因を指したメッセージで弾く
            throw std::invalid_argument("index is corrupted (negative ntotal)");
        }
        if (k > objIdx->ntotal) {
            // 番兵 (-1) は結果から除外するため k > ntotal は k = ntotal と同一の結果になる。
            // クランプしておくことで巨大な k による n * k の確保・乗算オーバーフローも防ぐ
            k = objIdx->ntotal;
        }
        if (0 == k) {
            // 空インデックス（ntotal == 0）への検索は、k の指定有無によらず空の結果を返す
            // （クランプ後に判定することで k 明示指定の経路でも faiss の k > 0 要求を踏まない）
            array_init(return_value);
            return;
        }

        // faiss は n * k 件書き込む。従来は k 件の VLA しか確保しておらず、
        // n >= 2 でスタックを踏み越え、n == 0 では未初期化のスタックを返していた。
        // なお n >= 2（複数クエリ）の結果はクエリ別に分かれず、ラベル単位で集約した
        // 1 本のランキングになる（Count = 何本のクエリでヒットしたか）
        faiss::idx_t n = number;
        if (static_cast<size_t>(n) > SIZE_MAX / static_cast<size_t>(k)) {
            // 通常運用では到達しない（n は実在配列由来・k は ntotal クランプ済み）が、
            // 破損・細工されたインデックスファイルの ntotal 由来で k が極端に
            // 大きい場合に n * k が wrap するのを防ぐ
            throw std::invalid_argument("n * k overflows size_t");
        }
        size_t total = static_cast<size_t>(n) * static_cast<size_t>(k);
        std::vector<float> distances(total);
        std::vector<faiss::idx_t> labels(total);

        objIdx->search(n, querys.data(), k, distances.data(), labels.data());

        // L2 等の距離系は昇順（近い順）、内積等の類似度系は降順（似ている順）
        const bool ascending = !faiss::is_similarity_metric(objIdx->metric_type);
        std::vector<croco::stats_t> stats = croco::FaissStatsFormat(distances.data(), labels.data(), total, ascending);
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

/* {{{ proto array faiss::reconstruct(int key)
 */
PHP_METHOD(croco_faiss_class, reconstruct)
{
    zend_long key;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(key)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    try {
        php_croco_faiss_object *faiObj = Z_FAISS_P(ZEND_THIS);
        const faiss::Index *objIdx = reinterpret_cast<const faiss::Index*>(faiObj->handle);
        if (objIdx->d <= 0) {
            throw std::invalid_argument("index dimension must be positive");
        }
        if (key == ZEND_LONG_MAX) {
            // faiss 内部の境界チェック (i0 + ni <= ntotal) は int64 の key + 1 が
            // wrap すると通過してしまう。wrap する唯一の値をここで弾く
            throw std::invalid_argument("key (" + std::to_string(key) + ") is out of range");
        }

        // faiss は d 個の float を書き込む。従来は呼び出し側の PHP 配列の要素数ぶんしか
        // 確保しておらず、d に満たない配列でヒープを踏み越えていた。さらに引数が
        // by-value のため書き込んだ結果は PHP 側へ返らなかった。
        // 入力配列は faiss に読まれもしないため受け取りをやめ、復元ベクトルを戻り値で返す
        std::vector<float> recons(static_cast<size_t>(objIdx->d));
        objIdx->reconstruct(key, recons.data());

        array_init(return_value);
        for (float v : recons) {
            add_next_index_double(return_value, static_cast<double>(v));
        }
    } catch (const std::exception& e) {
        zend_throw_exception(zend_ce_error_exception, e.what(), 0);
        RETURN_FALSE;
    }
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
