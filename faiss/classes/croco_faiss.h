#ifndef PHP_CLASSES_CROCO_FAISS_H
# define PHP_CLASSES_CROCO_FAISS_H

# ifdef __cplusplus

#include <cmath>
#include <string>
#include <vector>
#include <faiss/MetricType.h>
#include <faiss/index_factory.h>
#include <faiss/index_io.h>

extern "C" {

#include "php.h"
#include "ext/standard/info.h"
#include "php_croco_faiss.h"
#include "zend_exceptions.h"


# endif /* __cplusplus */

typedef void *FaissHandle;

typedef struct _php_croco_faiss_object {
    FaissHandle handle;
    zend_object zo;
} php_croco_faiss_object;

static inline php_croco_faiss_object *php_croco_faiss_from_obj(zend_object *obj) {
    return (php_croco_faiss_object*)((char*)(obj) - XtOffsetOf(php_croco_faiss_object, zo));
}

#define Z_FAISS_P(zv) php_croco_faiss_from_obj(Z_OBJ_P((zv)))

PHP_METHOD(croco_faiss_class, __construct);
PHP_METHOD(croco_faiss_class, __destruct);
PHP_METHOD(croco_faiss_class, isTrained);
PHP_METHOD(croco_faiss_class, add);
PHP_METHOD(croco_faiss_class, addWithIds);
PHP_METHOD(croco_faiss_class, ntotal);
PHP_METHOD(croco_faiss_class, search);
PHP_METHOD(croco_faiss_class, reset);
PHP_METHOD(croco_faiss_class, reconstruct);
PHP_METHOD(croco_faiss_class, writeIndex);
PHP_METHOD(croco_faiss_class, readIndex);
PHP_METHOD(croco_faiss_class, importIndex);
PHP_METHOD(croco_faiss_class, exportIndex);

# ifdef __cplusplus
}   // extern "C"


# endif /* __cplusplus */

#endif /* PHP_CLASSES_CROCO_FAISS_H */