#ifndef PHP_CLASSES_CROCO_EMBEDDING_H
# define PHP_CLASSES_CROCO_EMBEDDING_H

# ifdef __cplusplus

#include <cmath>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include "embedding.hpp"

extern "C" {

#include "php.h"
#include "ext/standard/info.h"
#include "php_croco_embedding.h"
#include "zend_exceptions.h"

# endif /* __cplusplus */

typedef void *EmbeddingHandle;

typedef struct _php_croco_embedding_object {
    EmbeddingHandle handle;
    zend_object zo;
} php_croco_embedding_object;

static inline php_croco_embedding_object *php_croco_embedding_from_obj(zend_object *obj) {
    return (php_croco_embedding_object*)((char*)(obj) - XtOffsetOf(php_croco_embedding_object, zo));
}

#define Z_EMBEDDING_P(zv) php_croco_embedding_from_obj(Z_OBJ_P((zv)))

PHP_METHOD(croco_embedding_class, __construct);
PHP_METHOD(croco_embedding_class, __destruct);
PHP_METHOD(croco_embedding_class, decode);
PHP_METHOD(croco_embedding_class, decodeList);

# ifdef __cplusplus
}   // extern "C"


# endif /* __cplusplus */

#endif /* PHP_CLASSES_CROCO_EMBEDDING_H */