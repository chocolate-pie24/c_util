/**
 * @file core_string_internal_data.h
 * @brief core_string_tの内部実装に関する構造体定義（非公開ヘッダ）
 *
 * このヘッダファイルは、core_stringモジュール内部で使用される
 * core_string_internal_data_t構造体を定義する。
 * API利用者がこのヘッダを直接インクルードする必要はない。
 *
 * @note 内部用ヘッダであり、公開インターフェースでは使用しないこと。
 */

#pragma once

#include <stdint.h>

/**
 * @struct core_string_internal_data_t
 * @brief core_string_tの内部構造体。文字列バッファとメタ情報を保持する。
 *
 * この構造体は core_string_t の実装における内部状態を表す。
 * 利用者が直接この構造体にアクセスすることは想定されておらず、
 * core_string.c内でのみ使用される。
 *
 */
typedef struct core_string_internal_data_t {
    char* buffer;       /**< ヌル終端された文字列バッファ */
    uint64_t length;    /**< 文字列長（終端文字を除く） */
    uint64_t buff_size; /**< バッファサイズ（終端文字含む） */
} core_string_internal_data_t;
