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
 * @section prerequisites 初期化に関する注意点
 * 一部の関数は @ref dynamic_array_create() によって初期化された dynamic_array_t を必要とします。
 * 未初期化の dynamic_array_t を渡すとエラーコードを返す設計になっています。
 * 詳細は各関数の @note セクションを参照してください。
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
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 100, &test_array);   // 100個の要素を格納できるバッファを確保
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
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 0, &test_array);   // メモリ確保はここでは行わない
 * DYNAMIC_ARRAY_ERROR_CODE result_reserve = dynamic_array_reserve(100, &test_array);   // 配列要素100個分のメモリを確保
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] max_element_count_ 格納する配列要素の数
 * @param[out] dynamic_array_ メモリ確保対象オブジェクト
 *
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_がNULL
 * @retval DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR 必要なメモリ領域の確保に失敗
 * @retval DYNAMIC_ARRAY_SUCCESS メモリ確保が正常に終了または必要なメモリ確保量が0で何もしなかった
 *
 * @see core_free()
 * @see core_malloc()
 * @see core_zero_memory()
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_reserve(uint64_t max_element_count_, dynamic_array_t* const dynamic_array_);

/**
 * @brief 与えられた dynamic_array_ の内部バッファサイズを指定された要素数にリサイズする
 *
 * @note dynamic_array_resize()は、「現在格納されている要素数以上」への拡張のみサポートする。
 * より小さい容量へ縮小する場合は、別途新しいdynamic_array_tオブジェクトを作成し、そちらに値をコピーすること。
 * なお、より小さい容量を指定した際には、
 * ```
 * [WARNING] dynamic_array_resize - Cannot resize to smaller max_element_count than current element_count.
 * ```
 * DYNAMIC_ARRAY_INVALID_ARGUMENT を返すとともに、上記のような [WARNING] ログが出力される。
 *
 * @note 本関数内でのバッファメモリの取得には、 @ref dynamic_array_reserve() を使用している。
 * このため、dynamic_array_resize()を行う前には @ref dynamic_array_create() によって、格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。
 * 詳細は @ref dynamic_array_reserve() を参照のこと。
 *
 * 使用例:
 * @code
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 16, &test_array);   // 配列要素16個分のメモリを確保
 * // エラー処理
 * DYNAMIC_ARRAY_ERROR_CODE result_resize = dynamic_array_resize(128, &test_array);   // 配列要素128個分にバッファを拡張
 * // エラー処理
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] max_element_count_ 拡張後に格納可能となる配列要素数
 * @param[out] dynamic_array_ 拡張対象オブジェクト
 *
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_がNULLまたは現在保持する容量よりも小さいサイズが指定された
 * @retval DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR データ一時退避用バッファまたは拡張後のバッファのメモリ取得に失敗
 * @retval DYNAMIC_ARRAY_SUCCESS バッファの拡張に成功し正常終了
 *
 * @see dynamic_array_reserve()
 * @see core_malloc()
 * @see core_free()
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_resize(uint64_t max_element_count_, dynamic_array_t* const dynamic_array_);

/**
 * @brief 与えられたdynamic_array_に格納可能な配列要素数を取得する
 *
 * @note 本APIを使用するためには、dynamic_array_tオブジェクトに対し、 @ref dynamic_array_create() によって、
 * 格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。初期化されていない状態で本APIを呼び出した場合は、
 * ```
 * [ERROR] dynamic_array_capacity - Provided dynamic_array_ is not initialized. Call dynamic_array_create.
 * ```
 * 上記のエラーメッセージを出力するとともに、DYNAMIC_ARRAY_INVALID_DARRAYが返される。
 *
 *
 * 使用例:
 * @code
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 64, &test_array);   // 配列用数64で初期化
 * // エラー処理
 * uint64_t capacity = 0;
 * DYNAMIC_ARRAY_ERROR_CODE result_capacity = dynamic_array_capacity(&test_array, &capacity);   // capacityに64が格納される
 * // エラー処理
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] dynamic_array_ 配列要素数取得元オブジェクト
 * @param[out] out_capacity_ 配列要素数格納先
 *
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_またはout_capacity_がNULL
 * @retval DYNAMIC_ARRAY_INVALID_DARRAY 未初期化のdynamic_array_が渡された
 * @retval DYNAMIC_ARRAY_SUCCESS 格納可能な配列要素数の取得に成功し、正常終了
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_capacity(const dynamic_array_t* const dynamic_array_, uint64_t* const out_capacity_);

/**
 * @brief 与えられたdynamic_array_に既に格納されている配列要素数を取得する
 *
* @note 本APIを使用するためには、dynamic_array_tオブジェクトに対し、 @ref dynamic_array_create() によって、
 * 格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。初期化されていない状態で本APIを呼び出した場合は、
 * ```
 * [ERROR] dynamic_array_size - Provided dynamic_array_ is not initialized. Call dynamic_array_create.
 * ```
 * 上記のエラーメッセージを出力するとともに、DYNAMIC_ARRAY_INVALID_DARRAYが返される。
 *
 *
 * 使用例:
 * @code
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 64, &test_array);   // 配列用数64で初期化
 *
 * element_data_t data;
 * DYNAMIC_ARRAY_ERROR_CODE result_push = DYNAMIC_ARRAY_SUCCESS;
 * result_push = dynamic_array_element_push(&data, &test_array);    // 要素を追加(エラー処理は省略)
 * result_push = dynamic_array_element_push(&data, &test_array);    // 要素を追加(エラー処理は省略)
 * result_push = dynamic_array_element_push(&data, &test_array);    // 要素を追加(エラー処理は省略)
 *
 * uint64_t size = 0;
 * DYNAMIC_ARRAY_ERROR_CODE result_capacity = dynamic_array_size(&test_array, &size);   // サイズに3が格納される
 * // エラー処理
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] dynamic_array_ 格納済み配列要素数取得対象オブジェクト
 * @param[out] out_size_ 格納済みの配列要素数
 *
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_またはout_size_がNULL
 * @retval DYNAMIC_ARRAY_INVALID_DARRAY 未初期化のdynamic_array_が渡された
 * @retval DYNAMIC_ARRAY_SUCCESS 格納済みの配列要素数の取得に成功し、正常終了
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_size(const dynamic_array_t* const dynamic_array_, uint64_t* const out_size_);

/**
 * @brief dynamic_array_のバッファに対し、新たな要素を追加する。
 *
 * @note 本APIを使用するためには、dynamic_array_tオブジェクトに対し、 @ref dynamic_array_create() によって、
 * 格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。初期化されていない状態で本APIを呼び出した場合は、
 * ```
 * [ERROR] dynamic_array_element_push - Provided dynamic_array_ is not initialized. Call dynamic_array_create.
 * ```
 * 上記のエラーメッセージを出力するとともに、DYNAMIC_ARRAY_INVALID_DARRAYが返される。
 *
 *
 * 使用例:
 * @code
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 64, &test_array);   // 配列用数64で初期化
 *
 * element_data_t data;
 * DYNAMIC_ARRAY_ERROR_CODE result_push = dynamic_array_element_push(&data, &test_array);    // 要素を追加
 * // エラー処理
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] object_ 追加オブジェクト
 * @param[in,out] dynamic_array_ 要素を追加する対象の配列オブジェクト（内部状態が更新される）
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数object_またはdynamic_array_がNULL
 * @retval DYNAMIC_ARRAY_INVALID_DARRAY 未初期化のdynamic_array_が渡された
 * @retval DYNAMIC_ARRAY_BUFFER_FULL 格納先のバッファが既に満杯
 * @retval DYNAMIC_ARRAY_SUCCESS バッファへの追加に成功し、正常終了
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_push(const void* const object_, dynamic_array_t* const dynamic_array_);

/**
 * @brief dynamic_array_の内部バッファの配列インデックスelement_index_に格納されているデータをout_object_に格納する。
 *
 * @note 本APIを使用するためには、dynamic_array_tオブジェクトに対し、 @ref dynamic_array_create() によって、
 * 格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。初期化されていない状態で本APIを呼び出した場合は、
 * ```
 * [ERROR] dynamic_array_element_ref - Provided dynamic_array_ is not initialized.
 * ```
 * 上記のエラーメッセージを出力するとともに、DYNAMIC_ARRAY_INVALID_DARRAYが返される。
 *
 *
 * 使用例:
 * @code
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 64, &test_array);   // 配列用数64で初期化
 *
 * element_data_t data = {10, 20, 30};
 * DYNAMIC_ARRAY_ERROR_CODE result_push = dynamic_array_element_push(&data, &test_array);
 * // エラー処理
 *
 * element_data_t out_data;
 * DYNAMIC_ARRAY_ERROR_CODE result_ref = dynamic_array_element_ref(0, &test_array, &out_data);    // out_dataにdataの内容がコピーされる
 * // エラー処理
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] element_index_ 取得したいオブジェクトが格納されている配列インデックス
 * @param[in] dynamic_array_ 取得元オブジェクト
 * @param[out] out_object_ 取得したオブジェクト格納先
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_またはout_object_がNULL
 * @retval DYNAMIC_ARRAY_INVALID_DARRAY 未初期化のdynamic_array_が渡された
 * @retval DYNAMIC_ARRAY_OUT_OF_RANGE 保有している配列の範囲外のインデックスが渡された
 * @retval DYNAMIC_ARRAY_SUCCESS オブジェクトの取得に成功し、正常終了
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_ref(uint64_t element_index_, const dynamic_array_t* const dynamic_array_, void* const out_object_);

/**
 * @brief dynamic_array_の内部バッファに格納されているオブジェクトを配列インデックスを指定して上書きする
 *
 * @note 本APIを使用するためには、dynamic_array_tオブジェクトに対し、 @ref dynamic_array_create() によって、
 * 格納するオブジェクトのサイズ、アライメント要件が初期化されている必要がある。初期化されていない状態で本APIを呼び出した場合は、
 * ```
 * [ERROR] dynamic_array_element_set - Provided dynamic_array_ is not initialized.
 * ```
 * 上記のエラーメッセージを出力するとともに、DYNAMIC_ARRAY_INVALID_DARRAYが返される。
 *
 *
 * 使用例:
 * @code
 * typedef struct element_data_t {
 *     uint16_t data1;
 *     uint16_t data2;
 *     uint16_t data3;
 * } element_data_t;
 * dynamic_array_t test_array = DYNAMIC_ARRAY_INITIALIZER;
 * DYNAMIC_ARRAY_ERROR_CODE result_create = dynamic_array_create(sizeof(element_data_t), alignof(element_data_t), 64, &test_array);   // 配列用数64で初期化
 *
 * element_data_t dummy;
 * dynamic_array_element_push(&dummy, &test_array); // 1要素追加
 *
 * element_data_t data = {10, 20, 30};
 * DYNAMIC_ARRAY_ERROR_CODE result_set = dynamic_array_element_set(0, &data, &test_array);
 * // エラー処理
 * dynamic_array_destroy(&test_array);
 * @endcode
 *
 * @param[in] element_index_ 上書きするオブジェクトが格納されている配列インデックス
 * @param[in] object_ 上書き元のオブジェクト
 * @param[in,out] dynamic_array_ 上書き対象オブジェクトが格納されているオブジェクト
 * @retval DYNAMIC_ARRAY_INVALID_ARGUMENT 引数dynamic_array_またはobject_がNULL
 * @retval DYNAMIC_ARRAY_INVALID_DARRAY 未初期化のdynamic_array_が渡された
 * @retval DYNAMIC_ARRAY_OUT_OF_RANGE 保有している配列の範囲外のインデックスが渡された
 * @retval DYNAMIC_ARRAY_SUCCESS 上書きに成功し正常終了
 */
DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_set(uint64_t element_index_, void* object_, dynamic_array_t* const dynamic_array_);
