#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "php.h"
#include "ext/standard/info.h"

#include "php_croco_faiss.h"
#include "classes/croco_faiss.h"

/* Handlers */
static zend_object_handlers croco_faiss_object_handlers;

/* Class entries */
zend_class_entry *croco_faiss_ce;

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_ctor, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, dimension, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, description, IS_STRING, 0, "\"Flat\"")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, metric, IS_LONG, 0, "\\Croco\\Faiss\\METRIC_L2")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_dtor, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_vector, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, array, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, number, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_addwids, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, array, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, ids, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, number, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_search, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, array, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, k, IS_LONG, 0, "0")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, format, IS_LONG, 0, "0")
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, number, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_reconstruct, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, key, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_file, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, file, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_croco_faiss_data, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, file, IS_STRING, 0)
ZEND_END_ARG_INFO()

/* }}} */


/* {{{ php_class_methods */
static const zend_function_entry php_class_methods[] = {
    PHP_ME(croco_faiss_class, __construct, arginfo_croco_faiss_ctor,    ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(croco_faiss_class, __destruct,  arginfo_croco_faiss_dtor,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, isTrained,   arginfo_croco_faiss_void,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, add,         arginfo_croco_faiss_vector,  ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, addWithIds,  arginfo_croco_faiss_addwids, ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, ntotal,      arginfo_croco_faiss_void,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, search,      arginfo_croco_faiss_search,  ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, reset,       arginfo_croco_faiss_void,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, reconstruct, arginfo_croco_faiss_reconstruct, ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, writeIndex,  arginfo_croco_faiss_file,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, readIndex,   arginfo_croco_faiss_file,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, importIndex, arginfo_croco_faiss_data,    ZEND_ACC_PUBLIC)
	PHP_ME(croco_faiss_class, exportIndex, arginfo_croco_faiss_void,    ZEND_ACC_PUBLIC)

    PHP_FE_END
};
/* }}} */

static void php_object_free_storage(zend_object *object) /* {{{ */
{
    php_croco_faiss_object *intern = php_croco_faiss_from_obj(object);
    if (!intern) {
        return;
    }
    zend_object_std_dtor(&intern->zo);
}
/* }}} */

static zend_object *php_object_new(zend_class_entry *class_type) /* {{{ */
{
    php_croco_faiss_object *intern;

    intern = ecalloc(1, sizeof(php_croco_faiss_object) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->zo, class_type);
    object_properties_init(&intern->zo, class_type);
    intern->zo.handlers = &croco_faiss_object_handlers;

    return &intern->zo;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(croco_faiss)
{
    zend_class_entry ce;

    /* Register Faiss Class */
    memcpy(&croco_faiss_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    INIT_CLASS_ENTRY(ce, "Croco\\Faiss", php_class_methods);
    ce.create_object = php_object_new;
    croco_faiss_object_handlers.offset = XtOffsetOf(php_croco_faiss_object, zo);
    croco_faiss_object_handlers.clone_obj = NULL;
    croco_faiss_object_handlers.free_obj = php_object_free_storage;
    croco_faiss_ce = zend_register_internal_class(&ce);

    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_INNER_PRODUCT", 0, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_L2", 1, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_L1", 2, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_Linf", 3, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_Lp", 4, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_Canberra", 20, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_BrayCurtis", 21, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_JensenShannon", 22, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_Jaccard", 23, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_NaNEuclidean", 24, CONST_PERSISTENT | CONST_CS);
    REGISTER_LONG_CONSTANT("Croco\\Faiss\\METRIC_GOWER", 25, CONST_PERSISTENT | CONST_CS);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(croco_faiss)
{
    UNREGISTER_INI_ENTRIES();

#if defined(ZTS) && defined(COMPILE_DL_CROCO_FAISS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(croco_faiss)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "CROCO Faiss support", "enabled");
    php_info_print_table_row(2, "CROCO Faiss module version", PHP_CROCO_FAISS_VERSION);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ croco_faiss_module_entry
*/
zend_module_entry croco_faiss_module_entry = {
    STANDARD_MODULE_HEADER,
    "croco_faiss",
    NULL,
    PHP_MINIT(croco_faiss),
    PHP_MSHUTDOWN(croco_faiss),
    NULL,
    NULL,
    PHP_MINFO(croco_faiss),
    PHP_CROCO_FAISS_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CROCO_FAISS
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(croco_faiss)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */