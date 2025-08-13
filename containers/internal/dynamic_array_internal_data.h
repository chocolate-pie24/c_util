/**
 * @file dynamic_array_internal_data.h
 * @brief dynamic_array_tの内部実装に関する構造体定義（非公開ヘッダ）
 *
 * このヘッダファイルは、dynamic_arrayモジュール内部で使用される
 * dynamic_array_internal_data_t構造体を定義する。
 * API利用者がこのヘッダを直接インクルードする必要はない。
 *
 * @note 内部用ヘッダであり、公開インターフェースでは使用しないこと。
 */
#pragma once

#include <stdint.h>

/**
 * @struct dynamic_array_internal_data_t
 * @brief dynamic_array_tの内部構造体。バッファ管理データと格納するオブジェクトデータを格納する
 *
 * この構造体は dynamic_array_t の実装における内部状態を表す。
 * 利用者が直接この構造体にアクセスすることは想定されておらず、
 * dynamic_array.c内でのみ使用される。
 *
 */
typedef struct dynamic_array_internal_data_t {
    uint64_t element_size;          /**< 格納するオブジェクトのサイズ(byte) */
    uint64_t element_count;         /**< 現在memory_poolに格納されているオブジェクトの数 */
    uint64_t buffer_capacity;       /**< memory_poolのサイズ(byte) */
    uint64_t max_element_count;     /**< memory_poolに格納可能なオブジェクトの数 */
    uint64_t aligned_element_size;  /**< アライメントされた各オブジェクトに必要なメモリ領域 */
    uint8_t alignment_requirement;  /**< 格納するオブジェクトのメモリアラインメント要件 */
    alignas(8) void* memory_pool;   /**< @brief オブジェクト格納先バッファ */
} dynamic_array_internal_data_t;
