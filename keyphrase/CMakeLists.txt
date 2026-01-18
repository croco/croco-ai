cmake_minimum_required(VERSION 3.5.1)

project(croco_keyphrase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD_REQUIRED ON)

set(WARNING_FLAGS "-Wno-int-to-pointer-cast -Wno-implicit-function-declaration -Wno-builtin-declaration-mismatch")
set(CMAKE_C_FLAGS "-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -DPHP_ATOM_INC -DHAVE_CONFIG_H -O3 -funroll-loops")
set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})

execute_process(COMMAND "php-config84" "--prefix" OUTPUT_VARIABLE PHP_BASE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "php-config84" "--include-dir" OUTPUT_VARIABLE PHP_INCLUDE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "php-config84" "--extension-dir" OUTPUT_VARIABLE EXTENSION_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

set(COMPILE_DL_CROCO_KEYPHRASE 1)
set(HAVE_CROCO_KEYPHRASE 1)
include(config/config.cmake)

set(ARCH_LIBRARY_PATH /usr/lib)
find_library(mecab-shared NAMES libmecab.so PATHS ${ARCH_LIBRARY_PATH})
find_library(faiss-shared NAMES libfaiss.so PATHS ${ARCH_LIBRARY_PATH})


include_directories(
    "${PHP_INCLUDE_PATH}"
    "${PHP_INCLUDE_PATH}/main"
    "${PHP_INCLUDE_PATH}/TSRM"
    "${PHP_INCLUDE_PATH}/Zend"
    "${PHP_INCLUDE_PATH}/ext"
    "${PHP_INCLUDE_PATH}/ext/date/lib"
    "${PHP_INCLUDE_PATH}/sapi"
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR})

add_library(croco_keyphrase-shared SHARED 
    ${CMAKE_SOURCE_DIR}/classes/croco_keyphrase.cc
    ${CMAKE_SOURCE_DIR}/php.c)

target_link_libraries(croco_keyphrase-shared
    ${mecab-shared}
    ${faiss-shared})

set_target_properties(croco_keyphrase-shared PROPERTIES PREFIX "" OUTPUT_NAME croco_keyphrase) 
install (TARGETS croco_keyphrase-shared DESTINATION ${EXTENSION_DIR} 
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                GROUP_EXECUTE GROUP_WRITE GROUP_READ 
                WORLD_EXECUTE WORLD_READ)

