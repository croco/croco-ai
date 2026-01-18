#ifndef PHP_CLASSES_CROCO_KEYPHRASE_H
# define PHP_CLASSES_CROCO_KEYPHRASE_H

# ifdef __cplusplus

#include <cmath>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include "sentence.hpp"
#include "lines.hpp"
#include "phrases.hpp"
#include "multipartiterank.hpp"

extern "C" {

#include "php.h"
#include "ext/standard/info.h"
#include "php_croco_keyphrase.h"
#include "zend_exceptions.h"

# endif /* __cplusplus */

typedef void *KeyphraseHandle;

typedef struct _php_croco_keyphrase_object {
    KeyphraseHandle handle;
    zend_object zo;
} php_croco_keyphrase_object;

static inline php_croco_keyphrase_object *php_croco_keyphrase_from_obj(zend_object *obj) {
    return (php_croco_keyphrase_object*)((char*)(obj) - XtOffsetOf(php_croco_keyphrase_object, zo));
}

#define Z_KEYPHRASE_P(zv) php_croco_keyphrase_from_obj(Z_OBJ_P((zv)))

PHP_METHOD(croco_keyphrase_class, __construct);
PHP_METHOD(croco_keyphrase_class, __destruct);
PHP_METHOD(croco_keyphrase_class, extract);
PHP_METHOD(croco_keyphrase_class, candidate);

# ifdef __cplusplus
}   // extern "C"


# endif /* __cplusplus */

#endif /* PHP_CLASSES_CROCO_KEYPHRASE_H */