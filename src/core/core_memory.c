/**
 * @file core_memory.c
 * @author chocolate-pie24
 * @brief メモリ関連処理実装
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "core/core_memory.h"

void core_zero_memory(void* const buff_, uint32_t buff_size_) {
    char* const tmp = buff_;
    for(uint32_t i = 0; i != buff_size_; ++i) {
        tmp[i] = 0;
    }
}

void* core_malloc(size_t memory_size_) {
    void* memory_pool = malloc(memory_size_);
    return memory_pool;
}

void core_free(void* memory_pool_) {
    free(memory_pool_);
}
