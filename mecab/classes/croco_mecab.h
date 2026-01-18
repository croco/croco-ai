#ifndef PHP_CLASSES_CROCO_MECAB_H
# define PHP_CLASSES_CROCO_MECAB_H

# ifdef __cplusplus

#include <string>
#include <vector>
#include <mecab.h>

extern "C" {

#include "php.h"
#include "ext/standard/info.h"
#include "php_croco_mecab.h"
#include "zend_exceptions.h"

# endif /* __cplusplus */

typedef void *TaggerHandle;

typedef struct _php_croco_mecab_object {
    TaggerHandle handle;
    zend_object zo;
} php_croco_mecab_object;

static inline php_croco_mecab_object *php_croco_mecab_from_obj(zend_object *obj) {
    return (php_croco_mecab_object*)((char*)(obj) - XtOffsetOf(php_croco_mecab_object, zo));
}

#define Z_MECAB_P(zv) php_croco_mecab_from_obj(Z_OBJ_P((zv)))

PHP_METHOD(croco_mecab_class, __construct);
PHP_METHOD(croco_mecab_class, __destruct);
PHP_METHOD(croco_mecab_class, parse);
PHP_METHOD(croco_mecab_class, wakati);
PHP_METHOD(croco_mecab_class, yomi);
PHP_METHOD(croco_mecab_class, tagger);

# ifdef __cplusplus
}   // extern "C"


# endif /* __cplusplus */

#endif /* PHP_CLASSES_CROCO_MECAB_H */