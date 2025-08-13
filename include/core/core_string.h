/**
 * @file core_string.h
 * @author chocolate-pie24
 * @brief core_string_tオブジェクトの定義と関連APIの宣言
 *
 * @details
 * core_string_tは、ユーザーが文字列長さやバッファ管理を意識することなく、
 * 安全かつ簡単に文字列操作を行えることを目的としたAPIである。
 *
 * 代表的な操作として以下が提供される:
 *
 * - 文字列のコピー
 * - 文字列の連結
 * - 文字列のトリミング
 *
 * @anchor core_string_initialization_rule
 * 本APIでは、core_string_t型の扱いにおいて以下の状態を区別する:
 *
 * - NULLポインタ: オブジェクト自体がNULLの場合(例: core_string_t* object = 0)
 * - 未初期化状態: オブジェクト自体は存在するが、メンバの初期化がされていない状態(例: core_string_t object;)
 * - デフォルト状態: オブジェクト内部管理データinternal_data == NULLの状態。使用前に明示的な初期化が必要。
 * - 初期化済み状態: internal_dataが有効な領域を指しており、APIでの使用が可能な状態。
 *
 * 未初期化状態のオブジェクトをAPIに渡すと未定義の動作を引き起こす可能性があるため、
 * 必ず次のいずれかの方法で「デフォルト状態」に初期化してから使用すること:
 * - @ref core_string_default_create()
 * - @ref CORE_STRING_INITIALIZER
 *
 * その後、以下のいずれかの関数で「初期化済み状態」に遷移させる必要がある:
 * - @ref core_string_create()
 * - @ref core_string_buffer_reserve()
 * - @ref core_string_buffer_resize()
 *
 * 再初期化する場合は、先に @ref core_string_destroy() を呼び、メモリを解放すること。
 *
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

// TODO: core_string_replace() 部分文字列の入れ替え

/**
 * @brief core_string_t関連処理が出力するエラーコード
 *
 */
typedef enum CORE_STRING_ERROR_CODE {
    CORE_STRING_SUCCESS,                /**< 正常終了 */
    CORE_STRING_INVALID_ARGUMENT,       /**< 引数異常 */
    CORE_STRING_RUNTIME_ERROR,          /**< 実行時エラー */
    CORE_STRING_BUFFER_EMPTY,           /**< 文字列バッファが空 */
    CORE_STRING_MEMORY_ALLOCATE_ERROR,  /**< メモリアロケートエラー */
} CORE_STRING_ERROR_CODE;

/**
 * @brief 文字列オブジェクト構造体
 *
 * 文字列データとそれに付随する管理情報を格納する。
 * オブジェクトの初期化については、 @ref core_string_initialization_rule を参照のこと。
 */
typedef struct core_string_t {
    void* internal_data;    /**< オブジェクト内部データ */
} core_string_t;

/** @brief オブジェクト初期化用マクロ
 *
 * 使用例:
 * @code
 *  core_string_t string = CORE_STRING_INITIALIZER;
 * @endcode
 */
#define CORE_STRING_INITIALIZER { 0 }

/**
 * @brief 引数で与えたstring_オブジェクトを「デフォルト状態」に初期化する。
 *
 * @note オブジェクトのデフォルト状態については、 @ref core_string_initialization_rule を参照のこと。
 *
 * @note 内部にデータを保持している初期化済みオブジェクトに対して本関数を直接呼ぶと、
 *       内部データのメモリが解放されず、メモリリークの原因となる可能性がある。
 *       再利用する場合は、必ず事前に @ref core_string_destroy() を呼んでメモリを解放してから使用すること。
 *
 * @note 引数string_にNULLを与えた場合には、以下のワーニングメッセージを出力し、処理を終了する。
 *       ```
 *       [WARNING] core_string_default_create - Argument string_ requires a valid pointer.
 *       ```
 *
 * 使用例:
 * @code
 * core_string_t string;
 * core_string_default_create(&string); // オブジェクトがデフォルト状態となる(internal_dataはNULLとなる)。
 * core_string_destroy(&string);
 * @endcode
 *
 * なお、上記コードは下記のコードと等価である。
 * @code
 * core_string_t string = CORE_STRING_INITIALIZER;
 * core_string_destroy(&string);
 * @endcode
 *
 * @param[in,out] string_ デフォルト状態とするオブジェクト
 *
 * @see core_string_destroy()
 * @see core_zero_memory()
 */
void core_string_default_create(core_string_t* const string_);

/**
 * @brief 引数のsrc_をコピーして、dst_オブジェクトを初期化済み状態にする。
 *
 * @note 本関数は、未初期化状態・デフォルト状態いずれのオブジェクトにも使用可能である。
 *       各状態の詳細については、@ref core_string_initialization_rule を参照のこと。
 *
 * @note この関数の内部では @ref core_string_destroy() が呼び出されるため、
 *       dst_がすでに初期化済みで内部にデータを保持している場合は、
 *       保持しているメモリがすべて解放された後に再初期化される。
 *
 * 使用例:
 * @code
 * core_string_t dst = CORE_STRING_INITIALIZER;                         // オブジェクトをデフォルト状態にする
 * CORE_STRING_ERROR_CODE result = core_string_create("Hello", &dst);   // dstにHelloが格納され、オブジェクトが初期化済み状態となる
 * if(CORE_STRING_SUCCESS != result) {
 *     // ここにエラー処理を書く
 * }
 * core_string_destroy(&dst);
 * @endcode
 *
 * @param[in]  src_ 初期化文字列(終端文字まで含めてコピーされる)
 * @param[out] dst_ 初期化対象オブジェクト
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数src_またはdst_がNULL
 * @retval CORE_STRING_RUNTIME_ERROR 文字列のコピーに失敗(これが発生したら本関数のバグ)
 * @retval CORE_STRING_MEMORY_ALLOCATE_ERROR 文字列バッファメモリの確保に失敗
 * @retval CORE_STRING_SUCCESS 正常に初期化とコピーが完了
 *
 * @see core_string_destroy()
 * @see core_string_default_create()
 * @see core_string_buffer_reserve()
 */
CORE_STRING_ERROR_CODE core_string_create(const char* const src_, core_string_t* const dst_);

/**
 * @brief コピー元src_の文字列内容を、コピー先dst_に複製する。
 *
 * @note
 * - dst_がデフォルト状態( @ref core_string_initialization_rule 参照)またはバッファサイズ不足の場合、必要なサイズでバッファを再確保する。
 * - dst_のバッファサイズが src_以上の場合、既存バッファを再利用して内容を上書きする。
 *
 * 使用例:
 * @code
 * core_string_t src = CORE_STRING_INITIALIZER;
 * core_string_t dst = CORE_STRING_INITIALIZER;
 *
 * core_string_create("Hello", &src);             // コピー元を作成
 * CORE_STRING_ERROR_CODE result = core_string_copy(&src, &dst); // src→dstへコピー
 * if (CORE_STRING_SUCCESS != result) {
 *     // エラー処理
 * }
 *
 * core_string_destroy(&src);
 * core_string_destroy(&dst);
 * @endcode
 *
 * @param[in]  src_ コピー元オブジェクト(初期化済み状態である必要あり)( @ref core_string_initialization_rule 参照)
 * @param[out] dst_ コピー先オブジェクト
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数src_またはdst_がNULL
 * @retval CORE_STRING_RUNTIME_ERROR コピー元src_がデフォルト状態( @ref core_string_initialization_rule 参照)、または文字列コピーに失敗
 * @retval CORE_STRING_BUFFER_EMPTY コピー元オブジェクトsrc_が内部で保持している文字列が空
 * @retval CORE_STRING_MEMORY_ALLOCATE_ERROR コピー先オブジェクトのメモリ確保に失敗
 * @retval CORE_STRING_SUCCESS 正常にコピーが完了
 *
 * @see core_string_create()
 * @see core_string_default_create()
 * @see core_string_buffer_capacity()
 * @see core_string_buffer_reserve()
 * @see core_zero_memory()
 * @see core_string_destroy()
 */
CORE_STRING_ERROR_CODE core_string_copy(const core_string_t* const src_, core_string_t* const dst_);

/**
 * @brief dst_にchar*型文字列をコピーする
 *
 * @note
 * - dst_デフォルト状態( @ref core_string_initialization_rule 参照)またはバッファサイズ不足の場合、必要なサイズでバッファを再確保する。
 * - dst_のバッファサイズが src_以上の場合、既存バッファを再利用して内容を上書きする。
 *
 * 使用例:
 * @code
 * core_string_t dst = CORE_STRING_INITIALIZER;
 * CORE_STRING_ERROR_CODE result = core_string_copy_from_char("Hello", &dst);
 * if(CORE_STRING_SUCCESS != result) {
 *     // エラー処理
 * }
 * core_string_destroy(&dst);
 * @endcode
 *
 * @param[in]  src_ コピー元文字列
 * @param[out] dst_ コピー先文字列オブジェクト(デフォルト状態または初期化済み状態である必要あり)( @ref core_string_initialization_rule 参照)
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数src_またはdst_がNULL
 * @retval CORE_STRING_MEMORY_ALLOCATE_ERROR コピー先オブジェクトのメモリ確保に失敗
 * @retval CORE_STRING_RUNTIME_ERROR 文字列のコピーに失敗
 * @retval CORE_STRING_SUCCESS 正常にコピーが完了
 *
 * @see core_string_buffer_reserve()
 * @see core_zero_memory()
 * @see core_string_destroy()
 */
CORE_STRING_ERROR_CODE core_string_copy_from_char(const char* const src_, core_string_t* const dst_);

/**
 * @brief string_が保持するメモリを破棄する。
 *
 * @note この関数を呼ぶことで、core_string_t型オブジェクトが保持しているinternal_dataのメモリおよび、
 *       internal_data内に保持しているメモリ領域が解放され、NULLが設定される。
 *       これにより、internal_dataにはNULLが設定される。なお、メモリの解放にはcore_free()を使用している。
 *
 * @note 本関数により破棄したオブジェクトを再度使用する場合には、下記の関数を使用する。
 * - @ref core_string_create()
 * - @ref core_string_default_create()
 *
 * @note 引数string_にNULLを与えた場合には、以下のワーニングメッセージを出力し、処理を終了する。
 *       ```
 *       [WARNING] core_string_destroy - Argument string_ requires a valid pointer.
 *       ```
 *
 * 使用例:
 * @code
 * core_string_t string = CORE_STRING_INITIALIZER;
 * core_string_destroy(&string);    // stringが保持するメモリが解放される
 * @endcode
 *
 * @param[in,out] string_ 破棄対象オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @see core_free()
 */
void core_string_destroy(core_string_t* const string_);

/**
 * @brief string_オブジェクトに指定サイズの文字列バッファを確保する。
 *
 * @note
 * - string_がデフォルト状態( @ref core_string_initialization_rule 参照)の場合、internal_data構造体を確保して初期化する
 * - すでに指定したサイズ以上のバッファが確保されている場合には何もせず成功を返す
 * - バッファの再確保が行われた場合、既存のデータはすべて破棄され、バッファ全体がクリアされる
 *
 * 使用例:
 * @code
 * core_string_t string = CORE_STRING_INITIALIZER;
 * CORE_STRING_ERROR_CODE result = core_string_buffer_reserve(128, &string);    // 128byte確保
 * if(CORE_STRING_SUCCESS != result) {
 *     // エラー処理
 * }
 * core_string_destroy(&string);
 * @endcode
 *
 * @param[in]  buffer_size_ 確保するバッファサイズ(byte)(終端文字を含むサイズを指定する)(文字列長 + 1)
 * @param[out] string_      メモリ確保対象オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数string_がNULL
 * @retval CORE_STRING_MEMORY_ALLOCATE_ERROR メモリ確保に失敗
 * @retval CORE_STRING_SUCCESS メモリ確保が正常に完了
 *
 * @see core_string_buffer_capacity()
 * @see core_malloc()
 * @see core_free()
 * @see core_string_destroy()
 * @see core_zero_memory()
 */
CORE_STRING_ERROR_CODE core_string_buffer_reserve(uint64_t buffer_size_, core_string_t* const string_);

/**
 * @brief core_string_tオブジェクトのバッファサイズをbuffer_size_に拡張する
 *
 * @note
 * - この関数は既存の文字列データを保持したままバッファサイズを拡張する
 * - すでに指定サイズ以上のバッファが確保されている場合は何もせず成功を返す
 * - オブジェクトがデフォルト状態の場合は @ref core_string_buffer_reserve() を呼び出して新規に確保する( @ref core_string_initialization_rule 参照)
 *
 * 使用例:
 * @code
 * core_string_t string = CORE_STRING_INITIALIZER;
 * core_string_copy_from_char("Hello", &string);         // 現在 "Hello" を保持
 * core_string_buffer_resize(128, &string);              // バッファを128byteに拡張
 * core_string_destroy(&string);
 * @endcode
 *
 * @param[in]     buffer_size_ 新しいバッファサイズ(byte)
 *                             終端文字を含めるため「文字列長 + 1」を指定すること
 * @param[in,out] string_ バッファサイズを拡張する対象オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数string_がNULL
 * @retval CORE_STRING_MEMORY_ALLOCATE_ERROR 一時バッファまたは新規バッファのメモリ確保に失敗
 * @retval CORE_STRING_RUNTIME_ERROR 一時バッファへのコピーまたは戻しに失敗
 * @retval CORE_STRING_SUCCESS 正常にリサイズが完了、または既に十分な容量が確保されている
 *
 * @see core_string_buffer_reserve()
 * @see core_string_buffer_capacity()
 * @see core_malloc()
 * @see core_free()
 */
CORE_STRING_ERROR_CODE core_string_buffer_resize(uint64_t buffer_size_, core_string_t* const string_);

/**
 * @brief core_string_tオブジェクトのバッファサイズを取得する。
 *
 * @note
 * - オブジェクトがデフォルト状態の場合は0を返す( @ref core_string_initialization_rule 参照)
 * - 引数がNULLの場合は @ref INVALID_VALUE_U64 を返す
 *
 * 使用例:
 * @code
 * core_string_t string = CORE_STRING_INITIALIZER;
 * uint64_t capacity = core_string_buffer_capacity(&string); // 未初期化のため0が返る
 * @endcode
 *
 * @param[in] string_ バッファサイズを取得する対象オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval INVALID_VALUE_U64 引数string_がNULL
 * @retval 0 オブジェクトがデフォルト状態、またはバッファサイズが0
 * @retval バッファサイズ(byte) 正常に取得できた場合のバッファサイズを返す
 *
 * @see core_string_buffer_reserve()
 * @see core_string_buffer_resize()
 */
uint64_t core_string_buffer_capacity(const core_string_t* const string_);

/**
 * @brief core_string_tオブジェクトが保持する文字列格納バッファが空かどうかを判定する
 *
 * @note
 * - 引数がNULLまたはデフォルト状態( @ref core_string_initialization_rule 参照)の場合は true を返す
 * - 長さが0の文字列を保持している場合も true を返す
 *
 * 使用例:
 * @code
 * core_string_t string = CORE_STRING_INITIALIZER;
 * if (core_string_is_empty(&string)) {
 *     // まだ初期化されていない or 空文字列
 * }
 * @endcode
 *
 * @param[in] string_ 判定対象オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval true オブジェクトがNULL / デフォルト状態( @ref core_string_initialization_rule 参照) / 空文字列である
 * @retval false 文字列が1文字以上存在する
 *
 * @see core_string_length()
 * @see core_string_buffer_capacity()
 */
bool core_string_is_empty(const core_string_t* const string_);

/**
 * @brief 2つのcore_string_tオブジェクトが保持する文字列が等しいかを比較する
 *
 * @note
 * - どちらかの引数がデフォルト状態またはNULLの場合はfalseを返す( @ref core_string_initialization_rule 参照)
 * - 比較はバッファの長さと各文字列の中身が完全一致するかどうかで判定される
 *
 * 使用例:
 * @code
 * core_string_t str1 = CORE_STRING_INITIALIZER;
 * core_string_t str2 = CORE_STRING_INITIALIZER;
 * core_string_create("Hello", &str1);
 * core_string_create("Hello", &str2);
 *
 * if(core_string_equal(&str1, &str2)) {
 *     // 文字列が一致
 * }
 * core_string_destroy(&str1);
 * core_string_destroy(&str2);
 * @endcode
 *
 * @param[in] string1_ 比較対象1。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 * @param[in] string2_ 比較対象2。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval true  2つの文字列が等しい
 * @retval false 引数がNULL/デフォルト状態( @ref core_string_initialization_rule 参照) / 文字列長や中身が一致しない場合
 *
 * @see core_string_create()
 * @see core_string_destroy()
 */
bool core_string_equal(const core_string_t* const string1_, const core_string_t* const string2_);

/**
 * @brief char*型文字列とcore_string_tオブジェクトが保持する文字列が等しいかを比較する
 *
 * @note
 * - 引数のどちらかが NULL またはデフォルト状態( @ref core_string_initialization_rule 参照)の場合は false を返す
 * - 比較は文字列長と各文字が完全一致するかどうかで判定される
 *
 * 使用例:
 * @code
 * core_string_t str2 = CORE_STRING_INITIALIZER;
 * core_string_create("Hello", &str2);
 *
 * if(core_string_equal_from_char("Hello", &str2)) {
 *     // 一致
 * }
 * core_string_destroy(&str2);
 * @endcode
 *
 * @param[in] str1_    比較対象の文字列
 * @param[in] string2_ 比較対象のcore_string_tオブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval true  文字列が等しい
 * @retval false 引数が NULL / デフォルト状態( @ref core_string_initialization_rule 参照) / 文字列長や中身が一致しない場合
 *
 * @see core_string_equal()
 * @see core_string_create()
 * @see core_string_destroy()
 */
bool core_string_equal_from_char(const char* const str1_, const core_string_t* const string2_);

/**
 * @brief core_string_tオブジェクトが保持している文字列の長さを取得する
 *
 * @note
 * - 文字列の長さは終端文字 ('\0') を含まない
 * - 引数が NULL の場合は @ref INVALID_VALUE_U64 を返す
 * - デフォルト状態( @ref core_string_initialization_rule 参照)のオブジェクトが渡された場合は 0 を返す
 *
 * 使用例:
 * @code
 * core_string_t str = CORE_STRING_INITIALIZER;
 * core_string_create("Hello", &str);
 * uint64_t len = core_string_length(&str); // len = 5
 * core_string_destroy(&str);
 * @endcode
 *
 * @param[in] string_ 長さを取得する対象のcore_string_tオブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval INVALID_VALUE_U64 引数が NULL の場合
 * @retval 0                 未初期化のオブジェクトが渡された場合
 * @retval その他             保持している文字列の長さ
 *
 * @see core_string_create()
 * @see core_string_destroy()
 */
uint64_t core_string_length(const core_string_t* const string_);

/**
 * @brief core_string_tオブジェクトが保持している文字列の先頭ポインタを取得する
 *
 * @note
 * - 戻り値は内部バッファへのポインタであり、呼び出し側が解放してはいけない
 * - オブジェクトがデフォルト状態( @ref core_string_initialization_rule 参照)、または引数が NULL の場合は NULL を返す
 * - 戻り値は不変ではなく、core_string_tの内容が変更されると無効になる可能性がある
 *
 * 使用例:
 * @code
 * core_string_t str = CORE_STRING_INITIALIZER;
 * core_string_create("Hello", &str);
 * const char* cstr = core_string_cstr(&str); // cstr = "Hello"
 * printf("%s\n", cstr);
 * core_string_destroy(&str);
 * @endcode
 *
 * @param[in] string_ c文字列を取得する対象のcore_string_tオブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval NULL 引数がNULLまたはデフォルト状態( @ref core_string_initialization_rule 参照)のオブジェクトが渡された場合
 * @retval その他 内部バッファの先頭アドレス
 *
 * @see core_string_create()
 * @see core_string_destroy()
 * @see core_string_length()
 */
const char* core_string_cstr(const core_string_t* const string_);

/**
 * @brief dst_の末尾にstring_の内容を連結する
 *
 * @note
 * - 連結の結果、dst_のバッファサイズが不足する場合は自動でリサイズが行われる
 * - string_およびdst_がデフォルト状態( @ref core_string_initialization_rule 参照)の場合はエラーを返す
 * - 連結後、dst_の内部のバッファ末尾には必ず終端文字 '\0' が付加される
 *
 * 使用例:
 * @code
 * core_string_t dst = CORE_STRING_INITIALIZER;
 * core_string_create("Hello", &dst);
 * core_string_t add = CORE_STRING_INITIALIZER;
 * core_string_create(" World", &add);
 *
 * CORE_STRING_ERROR_CODE result = core_string_concat(&add, &dst);
 * if (CORE_STRING_SUCCESS != result) {
 *     // エラー処理
 * }
 * printf("%s\n", core_string_cstr(&dst)); // Hello World
 *
 * core_string_destroy(&add);
 * core_string_destroy(&dst);
 * @endcode
 *
 * @param[in]  string_  連結する文字列 (コピー元)。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 * @param[out] dst_     連結結果を格納するオブジェクト (コピー先)。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 *
 * @retval CORE_STRING_INVALID_ARGUMENT string_またはdst_が NULL
 * @retval CORE_STRING_RUNTIME_ERROR string_ またはdst_がデフォルト状態( @ref core_string_initialization_rule 参照)、または一時バッファの生成やコピーに失敗した場合
 * @retval CORE_STRING_SUCCESS 連結が正常に完了
 *
 * @see core_string_buffer_capacity()
 * @see core_string_buffer_resize()
 * @see core_string_cstr()
 * @see core_string_destroy()
 */
CORE_STRING_ERROR_CODE core_string_concat(const core_string_t* const string_, core_string_t* const dst_);

/**
 * @brief src_ の [from_, to_] 範囲の部分文字列を dst_ にコピーする
 *
 * @note
 * - from_からto_ の範囲は両端を含むインデックスとして扱われる
 * - dst_のバッファサイズが不足している場合は自動でリサイズが行われる
 * - 部分文字列のコピー後、dst_の末尾には必ず終端文字'\0'が付加される
 *
 * 使用例:
 * @code
 * core_string_t src = CORE_STRING_INITIALIZER;
 * core_string_create("Hello World", &src);
 * core_string_t dst = CORE_STRING_INITIALIZER;
 *
 * CORE_STRING_ERROR_CODE result = core_string_substring_copy(&src, &dst, 6, 10);
 * if (CORE_STRING_SUCCESS != result) {
 *     // エラー処理
 * }
 * printf("%s\n", core_string_cstr(&dst)); // "World"
 *
 * core_string_destroy(&dst);
 * core_string_destroy(&src);
 * @endcode
 *
 * @param[in]  src_   部分文字列を抽出する元の文字列オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 * @param[out] dst_   抽出結果を格納する文字列オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 * @param[in]  from_  抽出を開始するインデックス (0-based)
 * @param[in]  to_    抽出を終了するインデックス (0-based, inclusive)
 *
 * @retval CORE_STRING_INVALID_ARGUMENT
 * - from_ または to_ が不正 (from_ > to_)
 * - to_ が src_ の範囲を超えている
 * @retval CORE_STRING_RUNTIME_ERROR
 * - src_または dst_がデフォルト状態( @ref core_string_initialization_rule 参照)
 * - バッファのリサイズに失敗
 * @retval CORE_STRING_SUCCESS 正常に部分文字列のコピーが完了
 *
 * @see core_string_buffer_capacity()
 * @see core_string_buffer_resize()
 * @see core_string_cstr()
 */
CORE_STRING_ERROR_CODE core_string_substring_copy(const core_string_t* const src_, core_string_t* const dst_, uint16_t from_, uint16_t to_);

/**
 * @brief 指定したトリム文字を取り除いた文字列をdst_にコピーする。
 *
 * @note
 * - 左端の連続するltrim_文字と右端の連続するrtrim_文字を削除する
 * - トリム後の文字列長が0になる場合、空文字列としてdst_に格納する
 * - 必要に応じてdst_のバッファサイズを再確保する
 *
 * 使用例:
 * @code
 * core_string_t src = CORE_STRING_INITIALIZER;
 * core_string_t dst = CORE_STRING_INITIALIZER;
 * core_string_create("  hello  ", &src);
 * CORE_STRING_ERROR_CODE result = core_string_trim(&src, &dst, ' ', ' ');
 * if (CORE_STRING_SUCCESS == result) {
 *     // dstには "hello" が格納される
 * }
 * core_string_destroy(&src);
 * core_string_destroy(&dst);
 * @endcode
 *
 * @param[in]  src_   トリム対象の文字列オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 * @param[out] dst_   トリム結果の出力先オブジェクト。未初期化状態は不可。( @ref core_string_initialization_rule 参照)
 * @param[in]  ltrim_ 左端のトリム対象文字
 * @param[in]  rtrim_ 右端のトリム対象文字
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数がNULL
 * @retval CORE_STRING_RUNTIME_ERROR    src_がデフォルト状態( @ref core_string_initialization_rule 参照)または長さ取得に失敗
 * @retval CORE_STRING_MEMORY_ALLOCATE_ERROR メモリ再確保に失敗
 * @retval CORE_STRING_SUCCESS          正常終了
 *
 * @see core_string_length()
 * @see core_string_substring_copy()
 * @see core_string_buffer_reserve()
 */
CORE_STRING_ERROR_CODE core_string_trim(const core_string_t* const src_, core_string_t* const dst_, char ltrim_, char rtrim_);

/**
 * @brief core_string_tオブジェクトの内容をint32_t型整数に変換する。
 *
 * @note
 * - 文字列が整数として解釈できない場合はエラーを返す
 * - 変換結果がint32_tの範囲外の場合もエラーを返す
 *
 * 使用例:
 * @code
 * core_string_t str = CORE_STRING_INITIALIZER;
 * core_string_create("1234", &str);
 * int32_t value = 0;
 * CORE_STRING_ERROR_CODE result = core_string_to_i32(&str, &value);
 * if (CORE_STRING_SUCCESS == result) {
 *     // value == 1234
 * }
 * core_string_destroy(&str);
 * @endcode
 *
 * @param[in]  string_   変換対象の文字列オブジェクト。( @ref core_string_initialization_rule 参照)
 * @param[out] out_value_ 変換結果を格納するポインタ
 *
 * @retval CORE_STRING_INVALID_ARGUMENT 引数string_またはout_value_がNULL
 * @retval CORE_STRING_RUNTIME_ERROR    string_がデフォルト状態( @ref core_string_initialization_rule 参照)、空文字列、変換失敗、または範囲外の値
 * @retval CORE_STRING_SUCCESS          正常に変換が完了
 *
 * @see strtol()
 */
CORE_STRING_ERROR_CODE core_string_to_i32(const core_string_t* const string_, int32_t* out_value_);
