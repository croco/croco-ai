#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "php.h"
#include "ext/standard/info.h"

#include "php_croco_embedding.h"
#include "classes/croco_embedding.h"

/* Handlers */
static zend_object_handlers croco_embedding_object_handlers;

/* Class entries */
zend_class_entry *croco_embedding_ce;

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_embedding_ctor, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, model_path, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, threads, IS_LONG, 4, "null")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, size, IS_LONG, 2048, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_embedding_dtor, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_embedding_get, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, text, IS_STRING, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ php_class_methods */
static const zend_function_entry php_class_methods[] = {
    PHP_ME(croco_embedding_class, __construct,   arginfo_croco_embedding_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(croco_embedding_class, __destruct,    arginfo_croco_embedding_dtor, ZEND_ACC_PUBLIC)
    PHP_ME(croco_embedding_class, getEmbeddings, arginfo_croco_embedding_get,  ZEND_ACC_PUBLIC)
    PHP_FE_END
};
/* }}} */

static void php_object_free_storage(zend_object *object) /* {{{ */
{
    php_croco_embedding_object *intern = php_croco_embedding_from_obj(object);
    if (!intern) {
        return;
    }
    zend_object_std_dtor(&intern->zo);
}
/* }}} */

static zend_object *php_object_new(zend_class_entry *class_type) /* {{{ */
{
    php_croco_embedding_object *intern;

    intern = ecalloc(1, sizeof(php_croco_embedding_object) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->zo, class_type);
    object_properties_init(&intern->zo, class_type);
    intern->zo.handlers = &croco_embedding_object_handlers;

    return &intern->zo;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(croco_embedding)
{
    zend_class_entry ce;

    /* Register Embedding Class */
    memcpy(&croco_embedding_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    INIT_CLASS_ENTRY(ce, "Croco\\Embedding", php_class_methods);
    ce.create_object = php_object_new;
    croco_embedding_object_handlers.offset = XtOffsetOf(php_croco_embedding_object, zo);
    croco_embedding_object_handlers.clone_obj = NULL;
    croco_embedding_object_handlers.free_obj = php_object_free_storage;
    croco_embedding_ce = zend_register_internal_class(&ce);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(croco_embedding)
{
    UNREGISTER_INI_ENTRIES();

#if defined(ZTS) && defined(COMPILE_DL_CROCO_EMBEDDING)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(croco_embedding)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "CROCO Embedding support", "enabled");
    php_info_print_table_row(2, "CROCO Embedding module version", PHP_CROCO_EMBEDDING_VERSION);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ croco_embedding_module_entry
*/
zend_module_entry croco_embedding_module_entry = {
    STANDARD_MODULE_HEADER,
    "croco_embedding",
    NULL,
    PHP_MINIT(croco_embedding),
    PHP_MSHUTDOWN(croco_embedding),
    NULL,
    NULL,
    PHP_MINFO(croco_embedding),
    PHP_CROCO_EMBEDDING_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CROCO_EMBEDDING
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(croco_embedding)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */