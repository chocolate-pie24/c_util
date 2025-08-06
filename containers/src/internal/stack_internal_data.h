#pragma once

#include <stdint.h>

typedef struct stack_internal_data_t {
    uint64_t element_size;          /**< 格納するオブジェクトのサイズ(byte) */
    uint64_t buffer_capacity;       /**< memory_poolのサイズ(byte) */
    uint64_t max_element_count;     /**< memory_poolに格納可能なオブジェクトの数 */
    uint64_t aligned_element_size;  /**< アライメントされた各オブジェクトに必要なメモリ領域 */
    uint64_t top_index;
    uint8_t alignment_requirement;  /**< 格納するオブジェクトのメモリアラインメント要件 */
    void* memory_pool;              /**< オブジェクト格納先バッファ */
} stack_internal_data_t;
