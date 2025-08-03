#pragma once

#include <stdint.h>

typedef struct dynamic_array_internal_data_t {
    /** @brief 格納するオブジェクトのサイズ(byte) */
    uint64_t element_size;

    /** @brief 現在memory_poolに格納されているオブジェクトの数 */
    uint64_t element_count;

    /** @brief memory_poolのサイズ(byte) */
    uint64_t buffer_capacity;

    /** @brief memory_poolに格納可能なオブジェクトの数 */
    uint64_t max_element_count;

    /** @brief アライメントされた各オブジェクトに必要なメモリ領域 */
    uint64_t aligned_element_size;

    /** @brief 格納するオブジェクトのメモリアラインメント要件 */
    uint8_t alignment_requirement;

    /** @brief オブジェクト格納先バッファ */
    alignas(8) void* memory_pool;
} dynamic_array_internal_data_t;