/**
 * @file dynamic_array.h
 * @author chocolate-pie24
 * @brief dynamic_array_tオブジェクトの定義と関連APIの宣言
 *
 * @details
 * dynamic_array_tは、C++におけるstd::vectorのように、
 * ユーザーが配列サイズを気にしなくても要素の追加、参照を
 * 安全かつ簡単に行えることを目的としたAPIである。
 *
 * ただし、以下の点はstd::vectorとは仕様が異なる:
 * - オブジェクトをpushした際、領域の拡張が必要となった際には、必要となる分だけ確保する。
 * - 配列サイズの拡張処理において、サイズが小さくなる方への拡張は許可しない。
 *
 * 代表的な操作として以下が提供される:
 * - 配列へ格納可能な要素数の予約(reserve)
 * - 配列サイズの拡張(サイズが大きくなる方のみ許容)(resize)
 * - 配列への要素の追加(push)
 * - 配列要素の参照(ref)
 * - 配列要素の更新(set)
 *
 * @version 0.1
 * @date 2025-08-01
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

/**
 * @brief dynamic_array_t関連処理が出力するエラーコード
 *
 */
typedef enum DYNAMIC_ARRAY_ERROR_CODE {
    DYNAMIC_ARRAY_SUCCESS = 0x00,                   /**< 正常終了 */
    DYNAMIC_ARRAY_INVALID_ARGUMENT = 0x01,          /**< 引数異常 */
    DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR = 0x02,     /**< メモリアロケートエラー */
    DYNAMIC_ARRAY_BUFFER_FULL = 0x03,               /**< バッファが満杯 */
    DYNAMIC_ARRAY_OUT_OF_RANGE = 0x04,              /**< 配列インデックス異常 */
    DYNAMIC_ARRAY_INVALID_DARRAY = 0x05,            /**< 無効な動的配列オブジェクト */
} DYNAMIC_ARRAY_ERROR_CODE;

/**
 * @brief dynamic_array APIの動的配列オブジェクト構造体
 *
 * 配列に格納するオブジェクトと、それに付随する管理データを格納する。
 * オブジェクトの初期化には以下のいずれかを使用する:
 * - DYNAMIC_ARRAY_INITIALIZER
 * - dynamic_array_default_create
 * - dynamic_array_create
 *
 */
typedef struct dynamic_array_t {
    alignas(8) void* internal_data; /**< オブジェクト内部データ */
} dynamic_array_t;

/** @brief オブジェクト初期化用マクロ
 *
 * 使用例:
 * @code
 * dynamic_array_t darray = DYNAMIC_ARRAY_INITIALIZER;
 * @endcode
 */
#define DYNAMIC_ARRAY_INITIALIZER { 0 }

/**
 * @brief 引数で与えたdynamic_array_オブジェクトを「デフォルト状態」に初期化する。
 *
 * @note オブジェクトのデフォルト状態とは、dynamic_array_t.internal_dataがNULLであり、
 *       いかなるデータも保持していない状態を指す。
 *
 * @note 内部にデータを保持している初期化済みオブジェクトに対して本関数を直接呼ぶと、
 *       内部データのメモリが解放されず、メモリリークの原因となる可能性がある。
 *       再利用する場合は、必ず事前に @ref dynamic_array_destroy() を呼んでメモリを解放してから使用すること。
 *
 * @note 引数dynamic_array_にNULLを与えた場合には、以下のワーニングメッセージを出力し、処理を終了する。
 *       ```
 *       [WARNING] %dynamic_array_default_create - Argument dynamic_array_ requires a valid pointer.
 *       ```
 *
 * 使用例:
 * @code
 * dynamic_array_t dynamic_array;
 * dynamic_array_default_create(&dynamic_array);
 * dynamic_array_destroy(&dynamic_array);
 * @endcode
 *
 * なお、上記コードは下記のコードと等価である。
 * @code
 * dynamic_array_t dynamic_array = DYNAMIC_ARRAY_INITIALIZER;
 * dynamic_array_destroy(&dynamic_array);
 * @endcode
 *
 * @param[in,out] dynamic_array_ 初期化対象オブジェクト
 *
 * @see dynamic_array_destroy()
 */
void dynamic_array_default_create(dynamic_array_t* const dynamic_array_);

/**
 * @brief dynamic_array_tに格納する要素のサイズ、アライメント要件、格納数を指定してdynamic_array_の内部メモリを確保し初期化する
 *
 * @note この関数の内部では @ref dynamic_array_destroy() が呼び出されるため、
 *       dynamic_array_がすでに初期化済みで内部にデータを保持している場合は、
 *       保持しているメモリがすべて解放された後に再初期化される。
 *
 * @note
 * 各配列要素の格納に必要なメモリサイズは、挿入要素のsizeof(object) + padding_sizeで決定される。
 * このため、配列要素数が100の場合、100 * (sizeof(object) + padding_size)のメモリ領域が必要となる。
 *
 * padding_sizeは、下記のように求められる:
 * diff = element_size_ % internal_data->alignment_requirement;            // アライメントのズレ量
 * padding_size = internal_data->alignment_requirement - diff;             // パディングサイズ
 * padding_size = padding_size % internal_data->alignment_requirement;     // ピッタリの時のために計算
 *
 * 使用例(配列格納要素のアライメント要件は2):
 * @code
 * typedef struct element_data {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result = dynamic_array_create(sizeof(element_data), alignof(element_data), 100, &test_array);   // 100個の要素を格納できるバッファを確保
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] element_size_ 格納する要素のサイズ(sizeof(object))
 * @param[in] alignment_requirement_ 格納する要素のアライメント要件(alignof(object))
 * @param[in] max_element_count_ 格納する要素の数
 * @param[out] dynamic_array_ 初期化対象オブジェクト
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_がNULLまたは
 *                                        引数element_size_が0または
 *                                        引数alignment_requirement_が0
 * @retval DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR 内部データ格納メモリの確保に失敗または
 *                                             配列要素を格納するメモリ領域の確保に失敗
 *
 * @see dynamic_array_destroy()
 * @see core_malloc()
 * @see core_zero_memory()
 * @see dynamic_array_reserve()
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_create(uint64_t element_size_, uint8_t alignment_requirement_, uint64_t max_element_count_, dynamic_array_t* const dynamic_array_);

/**
 * @brief 引数で与えた動的配列オブジェクトdynamic_array_が保持するメモリを破棄する。
 *
 * @note この関数を呼ぶことで、dynamic_array_t型オブジェクトが保持しているinternal_dataのメモリおよび、
 *       internal_data内に保持しているメモリ領域が解放される。
 *       これにより、internal_dataにはNULLが設定される。なお、メモリの解放にはcore_free()を使用している。
 *
 * @note 本関数により破棄したオブジェクトを再度使用する場合には、下記の関数を使用する。
 * - @ref dynamic_array_default_create()
 * - @ref dynamic_array_create()
 *
 * @note 引数dynamic_array_にNULLを与えた場合には、以下のワーニングメッセージを出力し、処理を終了する。
 *       ```
 *       [WARNING] dynamic_array_destroy - Argument dynamic_array_ requires a valid pointer.
 *       ```
 *
 * 使用例:
 * @code
 * dynamic_array_t darray = DYNAMIC_ARRAY_INITIALIZER;
 * dynamic_array_destroy(&darray);    // darrayが保持するメモリが解放される
 * @endcode
 *
 * @param[in,out] dynamic_array_ 破棄対象オブジェクト
 *
 * @see core_free()
 */
void dynamic_array_destroy(dynamic_array_t* const dynamic_array_);

/**
 * @brief 引数で与えたdynamic_array_に対し、配列要素がmax_element_count_分格納可能なメモリ領域を確保する。
 *
 * @note 引数max_element_count_に0を指定した場合は、下記のワーニングメッセージを出力し、正常終了する。
 *         ```
 *         [WARNING] dynamic_array_reserve - Argument max_element_count_ is 0. Nothing to be done.
 *         ```
 *
 * @note 引数に与えたdynamic_array_が初期化済みで、すでに内部にデータを保持している場合には、
 *       保持している領域を解放し、再度必要なメモリ量を再取得する。この場合、ワーニングメッセージは出力されない。
 *
 * @note 本APIを使用するためには、 @ref dynamic_array_create() によって、格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。
 *       そのため、 @ref dynamic_array_create() 実行時にすでに必要な配列要素数が定まっている場合には本APIを使用する必要はない。
 *       本APIを使用するタイミングは、 @ref dynamic_array_create() 実行時に配列要素数が不明であり、確定したのちにメモリ確保を行う場合である。
 *       なお、その場合、** @ref dynamic_array_resize() を実行した時と同等の効果が得られるが、本APIを使用する方が効率が良い。**
 *
 * 使用例(配列格納要素のアライメント要件は2):
 * @code
 * typedef struct element_data {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data), alignof(element_data), 0, &test_array);   // メモリ確保はここでは行わない
 * DYNAMIC_ARRAY_ERROR_CODE result_reserve = dynamic_array_reserve(100, &test_array);   // 配列要素100個分のメモリを確保
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] max_element_count_ 格納する配列要素の数
 * @param[out] dynamic_array_ メモリ確保対象オブジェクト
 *
 * @retval DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR 必要なメモリ領域の確保に失敗
 * @retval DYNAMIC_ARRAY_SUCCESS メモリ確保が正常に終了または必要なメモリ確保量が0で何もしなかった
 *
 * @see core_free()
 * @see core_malloc()
 * @see core_zero_memory()
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_reserve(uint64_t max_element_count_, dynamic_array_t* const dynamic_array_);

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_resize(uint64_t max_element_count_, dynamic_array_t* const dynamic_array_);

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_capacity(const dynamic_array_t* const dynamic_array_, uint64_t* const out_capacity_);

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_size(const dynamic_array_t* const dynamic_array_, uint64_t* const out_size_);

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_push(const void* const object_, dynamic_array_t* const dynamic_array_);

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_ref(uint64_t element_index_, const dynamic_array_t* const dynamic_array_, void* const out_object_);

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_set(uint64_t element_index_, void* object_, dynamic_array_t* const dynamic_array_);
