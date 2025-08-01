/**
 * @file core_memory.h
 * @author chocolate-pie24
 * @brief メモリ関連処理定義
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 対象バッファを全て0でクリアする
 * @note できるだけ標準ライブラリを使用しないで自作で行きたいので自作した
 *
 * @param buff_ クリア対象バッファ
 * @param buff_size_ クリアバッファサイズ(byte)
 */
void core_zero_memory(void* buff_, uint32_t buff_size_);

/**
 * @brief 要求されたメモリを確保し、出力する
 * @note TODO: メモリトラッキング用メモリ種別追加
 *
 * @param memory_size_ 確保メモリ領域
 * @return void* 確保されたメモリ領域へのポインタ
 */
void* core_malloc(size_t memory_size_);

/**
 * @brief 指定されたメモリ領域を破棄する
 * @note TODO: メモリトラッキング用メモリ種別追加
 *
 * @param memory_pool_ 破棄対象メモリ領域
 */
void core_free(void* memory_pool_);
