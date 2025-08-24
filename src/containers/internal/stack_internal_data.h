#pragma once

#include <stdint.h>

#include "containers/stack.h"

/**
 * @brief スタックオブジェクト構造体
 *
 * スタックに格納するオブジェクトとそれに付随する管理情報を格納する。
 * オブジェクトの初期化については、下記を参照のこと。
 * - @ref stack_default_create()
 * - @ref stack_create()
 */
struct stack_t {
    size_t element_size;          /**< 格納するオブジェクトのサイズ(byte) */
    size_t buffer_size;           /**< memory_poolのサイズ(byte) */
    size_t max_element_count;     /**< memory_poolに格納可能なオブジェクトの数 */
    size_t aligned_element_size;  /**< アライメントされた各オブジェクトに必要なメモリ領域 */
    size_t top_index;
    size_t alignment_requirement;   /**< 格納するオブジェクトのメモリアラインメント要件 */
    uint8_t valid_flags;
    void* memory_pool;              /**< オブジェクト格納先バッファ */
};
