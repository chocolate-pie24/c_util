/**
 * @file stack.h
 * @author chocolate-pie24
 * @brief stack_tオブジェクトの定義と関連APIの宣言
 *
 * @details
 * stack_tは、スタックデータ構造を提供する。特徴は、
 * - 型を意識する必要なく、オブジェクトをpush / popする機能を提供
 * - オブジェクトのコピーを取得するpop( @ref stack_pop() )と、オブジェクトへの参照を取得する高速なpop( @ref stack_pop_peek_ptr() )を提供
 * - バッファのresize、reserveによるバッファサイズ拡張機能の提供(ただしpush時の動的拡張は行わない。ユーザーによる明示的な拡張が必要。)
 *
 * @anchor stack_initialization_rule
 * 本APIでは、stack_t型の扱いにおいて以下の状態を区別する:
 *
 * - NULLポインタ: オブジェクト自体がNULLの場合(例: core_string_t* object = 0)
 * - 未初期化状態: オブジェクト自体は存在するが、メンバの初期化がされていない状態(例: stack_t object;)
 * - デフォルト状態: オブジェクト内部管理データinternal_data == NULLの状態。使用前に明示的な初期化が必要。
 * - 初期化済み状態: internal_dataが有効な領域を指しており、APIでの使用が可能な状態。
 *
 * 未初期化状態のオブジェクトをAPIに渡すと未定義の動作を引き起こす可能性があるため、
 * 必ず次のいずれかの方法で「デフォルト状態」に初期化してから使用すること:
 *
 * - @ref stack_default_create()
 * - @ref STACK_INITIALIZER
 *
 * その後、以下の関数で「初期化済み状態」に遷移させる必要がある:
 *
 * - @ref stack_create()
 *
 * スレッド安全性:
 * - 本実装はスレッドセーフではない。必要に応じて呼び出し側で排他制御を行うこと。
 *
 * @version 0.1
 * @date 2025-08-10
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief stack_t関連処理が出力するエラーコード
 *
 */
typedef enum STACK_ERROR_CODE {
    STACK_ERROR_CODE_SUCCESS = 0x00,            /**< 正常終了 */
    STACK_ERROR_MEMORY_ALLOCATE_ERROR = 0x01,   /**< メモリアロケートエラー */
    STACK_ERROR_RUNTIME_ERROR = 0x02,           /**< 実行時エラー */
    STACK_ERROR_INVALID_ARGUMENT = 0x03,        /**< 引数異常 */
    STACK_ERROR_INVALID_STACK = 0x04,           /**< 無効なスタックオブジェクト */
    STACK_ERROR_STACK_EMPTY = 0x05,             /**< スタックが空 */
    STACK_ERROR_STACK_FULL = 0x06,              /**< スタックが満杯 */
} STACK_ERROR_CODE;

/**
 * @brief スタックオブジェクト構造体
 *
 * スタックに格納するオブジェクトとそれに付随する管理情報を格納する。
 * オブジェクトの初期化については、 @ref stack_initialization_rule を参照のこと。
 */
typedef struct stack_t {
    void* internal_data;    /**< オブジェクト内部データ */
} stack_t;

/** @brief オブジェクト初期化用マクロ
 *
 * 使用例:
 * @code
 *  stack_t stack = STACK_INITIALIZER;
 * @endcode
 */
#define STACK_INITIALIZER { 0 }

/**
 * @brief 引数で与えたstack_オブジェクトを「デフォルト状態」に初期化する。
 *
 * @note オブジェクトのデフォルト状態については、 @ref stack_initialization_rule を参照のこと。
 *
 * @note 内部にデータを保持している初期化済みオブジェクトに対して本関数を直接呼ぶと、
 *       内部データのメモリが解放されず、メモリリークの原因となる可能性がある。
 *       再利用する場合は、必ず事前に @ref stack_destroy() を呼んでメモリを解放してから使用すること。
 *
 * @note 引数stack_にNULLを与えた場合には、以下のワーニングメッセージを出力し、処理を終了する。
 *       ```
 *       [WARNING] stack_default_create - Argument stack_ requires a valid pointer.
 *       ```
 *
 * 使用例:
 * @code
 * stack_t stack;
 * stack_default_create(&stack); // オブジェクトがデフォルト状態となる(internal_dataはNULLとなる)。
 * stack_destroy(&stack);
 * @endcode
 *
 * なお、上記コードは下記のコードと等価である。
 * @code
 * stack_t stack = STACK_INITIALIZER;
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in,out] stack_ デフォルト状態とするオブジェクト
 *
 * @see stack_destroy()
 * @see core_zero_memory()
 */
void stack_default_create(stack_t* const stack_);

/**
 * @brief スタックに格納するオブジェクトのメモリ要件を指定してstack_を初期化する。
 *
 * @note 本関数によって初期化されたオブジェクト格納メモリ領域は、
 *
 * - @ref stack_resize() によって、内部データを保持したまま領域の拡張が可能である。
 * - @ref stack_reserve() によって領域の拡張が可能である(ただし、内部のデータは破棄される)。
 *
 * @note 本関数は、未初期化状態・デフォルト状態いずれのオブジェクトにも使用可能である。
 *       各状態の詳細については、@ref stack_initialization_rule を参照のこと。
 *
 * @note この関数の内部では @ref stac_destroy() が呼び出されるため、
 *       stack_がすでに初期化済みで内部にデータを保持している場合は、
 *       保持しているメモリがすべて解放された後に再初期化される。
 *
 * @note 引数alignment_requirement_は2の冪乗でなければいけない(1, 2, 4...)。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] element_size_ スタックに格納するオブジェクトのサイズ(sizeof(object)で取得する)
 * @param[in] alignment_requirement_ スタックに格納するオブジェクトのアライメント要件(alignof(object)で取得する)
 * @param[in] max_element_count_ スタックに格納するオブジェクトの数(1以上を指定する)
 * @param[out] stack_ 初期化対象オブジェクト
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT
 *                                      - 引数stack_がNULL
 *                                      - 引数element_size_、alignment_requirement_、max_element_count_に0が含まれる
 *                                      - max_element_count_の値が大きすぎる(バッファサイズの値がuint64_tの最大値を超過する)
 *                                      - alignment_requirement_が2の冪乗ではない
 * @retval STACK_ERROR_MEMORY_ALLOCATE_ERROR オブジェクト内部データまたはオブジェクト格納用メモリ領域の確保に失敗
 * @retval STACK_ERROR_CODE_SUCCESS オブジェクトの初期化に成功し、正常終了
 *
 * @see core_malloc()
 * @see core_zero_memory
 * @see stack_destroy()
 *
 * @see core_string_destroy()
 * @see core_string_default_create()
 * @see core_string_buffer_reserve()
 *
 * @todo TODO: if(0 != element_size_) {} によるエラーチェックとエラー処理は不要。関数頭ですでにチェック済み。
 * @todo TODO: if(0 != max_element_count_) {} によるエラーチェックとエラー処理は不要。関数頭ですでにチェック済み。
 */
STACK_ERROR_CODE stack_create(uint64_t element_size_, uint8_t alignment_requirement_, uint64_t max_element_count_, stack_t* const stack_);

/**
 * @brief stack_が保持するメモリを破棄する。
 *
 * @note この関数を呼ぶことで、stack_t型オブジェクトが保持しているinternal_dataのメモリおよび、
 *       internal_data内に保持しているメモリ領域が解放され、NULLが設定される。
 *
 * @note 本関数により破棄したオブジェクトを再度使用する場合には、下記の関数を使用する。
 *
 * - @ref stack_create()
 *
 * @note 引数string_にNULLを与えた場合には、以下のワーニングメッセージを出力し、処理を終了する。
 *       ```
 *       [WARNING] stack_destroy - Argument stack_ requires a valid pointer.
 *       ```
 *
 * 使用例:
 * @code
 * stack_t stack = STACK_INITIALIZER;
 * stack_destroy(&string);  // stackが保持するメモリが解放される
 * @endcode
 *
 * @param[in,out] stack_ 破棄対象オブジェクト。未初期化状態は不可。( @ref stack_initialization_rule 参照)
 *
 * @see core_free()
 */
void stack_destroy(stack_t* const stack_);

/**
 * @brief スタックオブジェクトのオブジェクト格納用メモリ領域を拡張または縮小する
 *
 * @note この関数を呼ぶことで、stack_t型オブジェクトが保持しているオブジェクト格納領域のメモリは破棄される。
 *       オブジェクトのデータを破棄しない場合は、 @ref stack_resize() を使用すること。
 *       なお、 @ref stack_resize() ではデータのコピー処理が必要であるため、オブジェクトを破棄して良い場合は本関数を使用する方が効率が良い。
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * STACK_ERROR_CODE result_reserve = stack_reserve(256, &stack); // オブジェクトを格納する数を256に拡張
 * if(STACK_ERROR_CODE_SUCCESS != result_reserve) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] max_element_count_ 拡張(or縮小)後のオブジェクト格納数
 * @param[out] stack_ 拡張(or縮小)対象スタックオブジェクト
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT
 *                                       - 引数stack_がNULL、またはmax_element_count_が0
 *                                       - max_element_count_の値が大きすぎる(バッファサイズの値がuint64_tの最大値を超過する)
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_MEMORY_ALLOCATE_ERROR オブジェクトを格納するメモリ領域の取得に失敗
 * @retval STACK_ERROR_CODE_SUCCESS オブジェクト格納用メモリ領域の確保に成功し、正常終了
 *
 * @see core_malloc()
 * @see core_free()
 * @see core_zero_memory()
 * @see stack_create()
 *
 */
STACK_ERROR_CODE stack_reserve(uint64_t max_element_count_, stack_t* const stack_);

/**
 * @brief スタックオブジェクトのオブジェクト格納用メモリ領域を内部のデータを保持したまま拡張する(縮小方向は不可)。
 *
 * @note 与えられたmax_element_count_が現在格納可能なオブジェクトの数以下であった場合は、以下のエラーメッセージを出力し、STACK_ERROR_INVALID_ARGUMENTを返す。
 * 現在保持しているデータを破棄しても良い場合は、 @ref stack_reserve() によってバッファの縮小が可能。
 * ```
 * [ERROR] stack_resize - Shrinking the buffer is not allowed.
 * ```
 *
 * @note 現在保持しているデータを破棄しても良い場合は、 @ref stack_reserve() を使用した方がデータのコピーが不要であるため処理が高速である。
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * STACK_ERROR_CODE result_reserve = stack_resize(256, &stack); // オブジェクトを格納する数を256に拡張
 * if(STACK_ERROR_CODE_SUCCESS != result_reserve) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] max_element_count_ 拡張(or縮小)後のオブジェクト格納数(0より大きく、かつ、現在格納可能なオブジェクト数より大きい値を指定する)
 * @param[out] stack_ 拡張対象スタックオブジェクト
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT
 *  - 引数stack_がNULLまたは
 *  - max_element_count_が0
 *  - max_element_count_が現在格納可能なオブジェクト数以下
 *  - max_element_count_の値が大きすぎる(バッファサイズの値がuint64_tの最大値を超過する)
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_MEMORY_ALLOCATE_ERROR 新しいサイズのバッファのメモリ確保に失敗
 * @retval STACK_ERROR_CODE_SUCCESS オブジェクト格納用メモリ領域の確保とコピーに成功し、正常終了
 *
 * @see core_malloc()
 * @see core_free()
 * @see core_zero_memory()
 * @see stack_reserve()
 * @see stack_create()
 *
 */
STACK_ERROR_CODE stack_resize(uint64_t max_element_count_, stack_t* const stack_);

/**
 * @brief stack_に対してdata_オブジェクトを追加する。
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object.sample_value1 = 1;
 * sample_object.sample_value2 = 2;
 * STACK_ERROR_CODE result_push = stack_push(&stack, &sample_object);   // データがpushされる
 * if(STACK_ERROR_CODE_SUCCESS != result_push) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in,out] stack_ オブジェクト追加対象スタック
 * @param[in] data_ 追加オブジェクト
 * @retval STACK_ERROR_INVALID_ARGUMENT 引数stack_NULLまたはdata_がNULL
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_STACK_FULL スタックが既に満杯でオブジェクトを追加できない
 * @retval STACK_ERROR_CODE_SUCCESS オブジェクトのスタックが成功し、正常終了
 *
 * @see stack_full()
 * @see stack_create()
 */
STACK_ERROR_CODE stack_push(stack_t* const stack_, const void* const data_);

/**
 * @brief stack_からデータを取り出し、out_data_に格納する(stack_の一番上のデータは削除される)
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object.sample_value1 = 1;
 * sample_object.sample_value2 = 2;
 * STACK_ERROR_CODE result_push = stack_push(&stack, &sample_object);   // データがpushされる
 * if(STACK_ERROR_CODE_SUCCESS != result_push) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object_t sample_object2 = { 0 };
 * STACK_ERROR_CODE result_pop = stack_pop(&stack, &sample_object2);    // sample_object2にsample_objectのデータがコピーされ、stackからsample_objectは削除される
 * if(STACK_ERROR_CODE_SUCCESS != result_pop) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in,out] stack_ データ取得元スタックオブジェクト
 * @param[out] out_data_ データ格納先バッファ
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT 引数stack_がNULLまたはout_data_がNULL
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_STACK_EMPTY スタックが空でデータを取り出せない
 * @retval STACK_ERROR_CODE_SUCCESS データの取得とコピーに成功し、正常終了
 *
 * @see stack_empty()
 * @see stack_create()
 */
STACK_ERROR_CODE stack_pop(const stack_t* const stack_, void* const out_data_);

/**
 * @brief stack_の一番上のデータへの参照を、out_data_に格納する(stack_の一番上のデータは削除されない)
 *
 * @note 本関数ではデータのコピーを必要としないため、高速にデータへの参照を取得できる。取得した後、スタックからデータを削除する場合は、 @ref stack_discard_top() を使用する。
 *
 * @note out_data_にはstackの一番上のデータへのポインタが格納されるが、const void*であるため、スタック内のデータおよびout_data_内のデータは書き換え不可である。
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object.sample_value1 = 1;
 * sample_object.sample_value2 = 2;
 * STACK_ERROR_CODE result_push = stack_push(&stack, &sample_object);   // データがpushされる
 * if(STACK_ERROR_CODE_SUCCESS != result_push) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object_t sample_object2 = { 0 };
 * STACK_ERROR_CODE result_pop = stack_pop_peek_ptr(&stack, &sample_object2);    // sample_object2にsample_objectのデータがコピーされ、stackのsample_objectは保持される
 * if(STACK_ERROR_CODE_SUCCESS != result_pop) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in,out] stack_ データ取得元スタックオブジェクト
 * @param[out] out_data_ データ格納先バッファ
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT 引数stack_がNULLまたはout_data_がNULL
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_STACK_EMPTY スタックが空でデータを取り出せない
 * @retval STACK_ERROR_CODE_SUCCESS データへの参照の取得に成功し、正常終了
 *
 * @see stack_empty()
 * @see stack_create()
 * @see stack_discard_top()
 * @see stack_pop()
 */
STACK_ERROR_CODE stack_pop_peek_ptr(const stack_t* const stack_, const void* *out_data_);

/**
 * @brief スタックに格納されている一番上のデータを破棄する
 *
 * @note 本関数は、 @ref stack_pop_peek_ptr() によってオブジェクトへの参照を取得した後、オブジェクトをスタックから削除するために用いる。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object.sample_value1 = 1;
 * sample_object.sample_value2 = 2;
 * STACK_ERROR_CODE result_push = stack_push(&stack, &sample_object);   // データがpushされる
 * if(STACK_ERROR_CODE_SUCCESS != result_push) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object_t sample_object2 = { 0 };
 * STACK_ERROR_CODE result_pop = stack_pop_peek_ptr(&stack, &sample_object2);    // sample_object2にsample_objectのデータがコピーされ、stackのsample_objectは保持される
 * if(STACK_ERROR_CODE_SUCCESS != result_pop) {
 *     // ここにエラー処理を書く
 * }
 *
 * STACK_ERROR_CODE result_discard = stack_discard_top(&stack); // stackからsample_objectが削除される
 * if(STACK_ERROR_CODE_SUCCESS != result_discard) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in,out] stack_ データ削除対象スタックオブジェクト
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT 引数stack_がNULL
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_STACK_EMPTY スタックが空でデータを削除できない
 * @retval STACK_ERROR_CODE_SUCCESS データの削除成功し、正常終了
 *
 * @see stack_pop_peek_ptr()
 * @see stack_empty
 */
STACK_ERROR_CODE stack_discard_top(const stack_t* const stack_);

// これは危険なので禁止
// STACK_ERROR_CODE stack_pop_ptr(const stack_t* const stack_, void** out_data_);

/**
 * @brief スタックに保存されているオブジェクトを全て消去する(オブジェクト格納バッファのメモリ領域は保持される)
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * sample_object.sample_value1 = 1;
 * sample_object.sample_value2 = 2;
 * STACK_ERROR_CODE result_push = stack_push(&stack, &sample_object);   // データがpushされる
 * if(STACK_ERROR_CODE_SUCCESS != result_push) {
 *     // ここにエラー処理を書く
 * }
 *
 * STACK_ERROR_CODE result_clear = stack_clear(&stack); // pushしたデータが削除される
 * if(STACK_ERROR_CODE_SUCCESS != result_clear) {
 *     // ここにエラー処理を書く
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in,out] stack_ オブジェクト消去対象スタック
 *
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @retval STACK_ERROR_CODE_SUCCESS スタックに可能しているオブジェクトの消去に成功し、正常終了
 *
 * @see stack_create()
 */
STACK_ERROR_CODE stack_clear(stack_t* const stack_);

/**
 * @brief スタックに格納可能なオブジェクトの数を取得する。
 *
 * @note この関数を呼ぶ前に、 @ref stack_create() によって、格納するオブジェクトのメモリ要件、アライメント要件、格納数が初期化されている必要がある。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;   // オブジェクトをデフォルト状態にする
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 *
 * uint64_t capacity = 0;
 * STACK_ERROR_CODE result_get_capacity = stack_capacity(&stack, &capacity);    // capacityに128が格納される
 * if(STACK_ERROR_CODE_SUCCESS != result_get_capacity) {
 *     // ここにエラー処理を書く
 * }
 *
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] stack_ 取得対象スタックオブジェクト
 * @param[out] out_capacity_ 格納可能なオブジェクト数の格納先
 *
 * @retval STACK_ERROR_INVALID_ARGUMENT 引数stack_がNULLまたはout_capacity_がNULL
 * @retval STACK_ERROR_INVALID_STACK スタックオブジェクトに必要なデータが与えられていない(デバッグモード(-DDEBUG_MODEを指定しビルド)で動作させることで未初期化変数の表示が可能)
 * @return STACK_ERROR_CODE_SUCCESS 格納可能なオブジェクトの数の取得に成功し、正常終了
 *
 * @see stack_create()
 */
STACK_ERROR_CODE stack_capacity(const stack_t* const stack_, uint64_t* const out_capacity_);

/**
 * @brief stack_で管理しているオブジェクトが満杯を判定する。
 *
 * @note stack_が初期化済み状態でない場合は、以下のメッセージを出力し、trueを返す。
 * 各状態の詳細については、@ref stack_initialization_rule を参照のこと。
 * ```
 * [WARNING] stack_empty - Provided stack is not valid.
 * ```
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 * if(!stack_full(&stack)) {    // この例ではまだスタックにデータをpushしていないためfalseとなる
 *     // スタックが満杯ではなかった場合の処理
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] stack_ 判定対象スタックオブジェクト
 *
 * @retval true スタックが満杯、もしくは初期化済み状態ではない。
 * @retval false スタックが満杯ではない。
 */
bool stack_full(const stack_t* const stack_);

/**
 * @brief stack_で管理しているオブジェクトが空かを判定する。
 *
 * @note stack_が初期化済み状態でない場合は、以下のメッセージを出力し、trueを返す。
 * 各状態の詳細については、@ref stack_initialization_rule を参照のこと。
 * ```
 * [WARNING] stack_empty - Provided stack is not valid.
 * ```
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 * if(stack_empty(&stack)) {    // この例ではまだスタックにデータをpushしていないためtrueとなる
 *     // スタックが空だった場合の処理
 * }
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] stack_ 判定対象スタックオブジェクト
 *
 * @retval true スタックが空、もしくは初期化済み状態ではない。
 * @retval false スタックが空ではない。
 */
bool stack_empty(const stack_t* const stack_);

/**
 * @brief スタックオブジェクトが出力するエラーコードを文字列にして出力する。
 *
 * @note 本関数に登録されていないエラーコードが渡された場合は、
 * "stack error code: undefined error."の文字列を出力する。
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     const char* error_message = stack_error_code_to_string(result);
 *     ERROR_MESSAGE("stack error message: '%s'.", error_message);
 * }
 * stack_debug_print(&stack);
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] err_code_ スタックオブジェクトが出力するエラーコード
 *
 * @return const char* エラーメッセージ
 */
const char* stack_error_code_to_string(STACK_ERROR_CODE err_code_);

/**
 * @brief デバッグ用にスタックオブジェクトの内部管理データを標準出力に出力する。
 *
 * @note 引数stack_がNULLの場合は、以下のメッセージを出し処理を終了する(デバッグビルド時のみ)。
 * ```
 * [DEBUG] Argument stack_ requires a valid pointer.
 * ```
 *
 * @note stack_がデフォルト状態で内部データがNULLの場合は、以下のワーニングメッセージを出し処理を終了する。
 * 各状態の詳細については、@ref stack_initialization_rule を参照のこと。
 * ```
 * [DEBUG] Provided stack is not initialized.
 * ```
 *
 * 使用例:
 * @code
 * typedef struct sample_object_t {
 *     uint16_t sample_value1;
 *     uint16_t sample_value2;
 * } sample_object_t;
 *
 * sample_object_t sample_object = { 0 };
 *
 * stack_t stack = STACK_INITIALIZER;
 * STACK_ERROR_CODE result = stack_create(sizeof(sample_object), alignof(sample_object), 128, &stack);   // sample_object_tを128個格納できるスタックとして初期化
 * if(STACK_ERROR_CODE_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 * stack_debug_print(&stack);
 * stack_destroy(&stack);
 * @endcode
 *
 * @param[in] stack_ 内部管理データを表示するスタックオブジェクト
 *
 */
void stack_debug_print(const stack_t* const stack_);
