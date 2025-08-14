/**
 * @file core_string.c
 * @author chocolate-pie24
 * @brief 文字列操作用API関数 core_string の実装ファイル
 *
 * @details
 * このファイルでは core_string_t オブジェクトを用いた文字列操作関数群を実装する。
 * core_stringは「ユーザーが文字列長さやバッファサイズを意識せずに
 * 安全に文字列操作を行えること」を目的としており、以下の特徴を持つ。
 *
 * - 動的メモリ管理を内部で行い、必要に応じてバッファサイズを自動拡張する
 * - 文字列コピー、連結、トリミング、比較、数値変換などの基本機能を提供
 * - 利用者は `core_string_destroy()` を呼ぶことで明示的にメモリを解放できる
 *
 * 利用上の注意:
 * - 使用後は必ず `core_string_destroy()` を呼び出し、内部のメモリを解放すること
 * - `core_string_default_create()` は未初期化オブジェクトをデフォルト状態に戻すための関数であり、
 *   すでに初期化済みのオブジェクトに対して呼び出すとメモリリークを起こす可能性がある
 *
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // for strtol
#include <limits.h> // for INT32_MAX

#include "core/core_string.h"
#include "core/core_memory.h"
#include "core/message.h"

#include "internal/core_string_internal_data.h"

#include "define.h"

static uint64_t pfn_string_length_from_char(const char* const str_);
static bool pfn_core_string_copy(const char* const src_, char* const dst_, uint64_t dst_buff_size_);

/**
 * @brief 引数のNULLチェックを行い、NULLであればCORE_STRING_INVALID_ARGUMENTで処理を終了するマクロ
 *
 */
#define CHECK_ARG_NULL_RETURN_ERROR(func_name_, arg_name_, ptr_) \
    if(0 == ptr_) { \
        ERROR_MESSAGE("%s - Argument %s requires a valid pointer.", func_name_, arg_name_); \
        return CORE_STRING_INVALID_ARGUMENT; \
    } \

/**
 * @brief 引数のNULLチェックを行い、NULLであればワーニングを出し、リターンするマクロ
 *
 */
#define CHECK_ARG_NULL_RETURN_VOID(func_name_, arg_name_, ptr_) \
    if(0 == ptr_) { \
        WARN_MESSAGE("%s - Argument %s requires a valid pointer.", func_name_, arg_name_); \
        return; \
    } \

void core_string_default_create(core_string_t* const string_) {
    CHECK_ARG_NULL_RETURN_VOID("core_string_default_create", "string_", string_);
    core_zero_memory(string_, sizeof(core_string_t));
}

CORE_STRING_ERROR_CODE core_string_create(const char* const src_, core_string_t* const dst_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_create", "src_", src_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_create", "dst_", dst_);

    // internal_dataとバッファを初期化し、メモリを確保する
    core_string_destroy(dst_);
    const uint64_t src_length = pfn_string_length_from_char(src_);
    const CORE_STRING_ERROR_CODE err_code_reserve = core_string_buffer_reserve(src_length + 1, dst_);
    if(CORE_STRING_SUCCESS != err_code_reserve) {
        ERROR_MESSAGE("core_string_create - Failed to reserve buffer memory.");
        return err_code_reserve;
    }

    // 引数の文字列データをコピー
    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(dst_->internal_data);
    internal_data->length = src_length;
    if(!pfn_core_string_copy(src_, internal_data->buffer, internal_data->length + 1)) {
        ERROR_MESSAGE("core_string_create - Failed to copy string.");
        core_string_destroy(dst_);
        return CORE_STRING_RUNTIME_ERROR;
    }
    return CORE_STRING_SUCCESS;
}

CORE_STRING_ERROR_CODE core_string_copy(const core_string_t* const src_, core_string_t* const dst_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_copy", "src_", src_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_copy", "dst_", dst_);

    if(0 == src_->internal_data) {
        ERROR_MESSAGE("core_string_copy - Provided string is not initialized.");
        return CORE_STRING_RUNTIME_ERROR;
    }

    core_string_internal_data_t* src_internal_data = (core_string_internal_data_t*)(src_->internal_data);
    if(0 == src_internal_data->buffer || 0 == src_internal_data->length) {
        ERROR_MESSAGE("core_string_copy - Provided string's buffer is empty.");
        return CORE_STRING_BUFFER_EMPTY;
    }

    const uint64_t src_capacity = core_string_buffer_capacity(src_);
    const uint64_t dst_capacity = core_string_buffer_capacity(dst_);
    if(src_capacity > dst_capacity || 0 == dst_->internal_data) {
        CORE_STRING_ERROR_CODE err_code_reserve = core_string_buffer_reserve(src_capacity, dst_);
        if(CORE_STRING_SUCCESS != err_code_reserve) {
            return err_code_reserve;
        }
    } else {
        core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(dst_->internal_data);
        core_zero_memory(internal_data->buffer, internal_data->buff_size);
    }

    core_string_internal_data_t* dst_internal_data = (core_string_internal_data_t*)(dst_->internal_data);
    if(!pfn_core_string_copy(src_internal_data->buffer, dst_internal_data->buffer, src_internal_data->length + 1)) {
        ERROR_MESSAGE("core_string_copy - Failed to copy buffer.");
        core_string_destroy(dst_);
        return CORE_STRING_RUNTIME_ERROR;
    }
    dst_internal_data->length = src_internal_data->length;
    return CORE_STRING_SUCCESS;
}

CORE_STRING_ERROR_CODE core_string_copy_from_char(const char* const src_, core_string_t* const dst_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_copy_from_char", "src_", src_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_copy_from_char", "dst_", dst_);

    const uint64_t src_length = pfn_string_length_from_char(src_);
    const uint64_t dst_capacity = core_string_buffer_capacity(dst_);
    if((src_length + 1) > dst_capacity || 0 == dst_->internal_data) {
        CORE_STRING_ERROR_CODE err_code_reserve = core_string_buffer_reserve((src_length + 1), dst_);
        if(CORE_STRING_SUCCESS != err_code_reserve) {
            return err_code_reserve;
        }
    } else {
        core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(dst_->internal_data);
        core_zero_memory(internal_data->buffer, internal_data->buff_size);
    }

    core_string_internal_data_t* dst_internal_data = (core_string_internal_data_t*)(dst_->internal_data);
    if(!pfn_core_string_copy(src_, dst_internal_data->buffer, (src_length + 1))) {
        ERROR_MESSAGE("core_string_copy_from_char - Failed to copy buffer.");
        core_string_destroy(dst_);
        return CORE_STRING_RUNTIME_ERROR;
    }
    dst_internal_data->length = src_length;
    return CORE_STRING_SUCCESS;
}

void core_string_destroy(core_string_t* const string_) {
    CHECK_ARG_NULL_RETURN_VOID("core_string_destroy", "string_", string_);
    if(0 != string_->internal_data) {
        core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
        if(0 != internal_data->buffer) {
            core_free(internal_data->buffer);
            internal_data->buffer = 0;
        }
        core_free(string_->internal_data);
        string_->internal_data = 0;
    }
}

CORE_STRING_ERROR_CODE core_string_buffer_reserve(uint64_t buffer_size_, core_string_t* const string_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_buffer_reserve", "string_", string_);
    if(0 != string_->internal_data) {
        const uint64_t capacity = core_string_buffer_capacity(string_);
        if(capacity >= buffer_size_) {
            DEBUG_MESSAGE("core_string_buffer_reserve - Requested buffer is already reserved. Requested size = %lld, Reserved size = %lld.", buffer_size_, capacity);
            return CORE_STRING_SUCCESS;
        }
    } else {
        string_->internal_data = core_malloc(sizeof(core_string_internal_data_t));
        if(0 == string_->internal_data) {
            ERROR_MESSAGE("core_string_buffer_reserve - Failed to allocate internal_data memory.");
            return CORE_STRING_MEMORY_ALLOCATE_ERROR;
        }
        core_zero_memory(string_->internal_data, sizeof(core_string_internal_data_t));
    }

    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
    core_free(internal_data->buffer);
    internal_data->buffer = core_malloc(buffer_size_);
    if(0 == internal_data->buffer) {
        ERROR_MESSAGE("core_string_internal_data_t - Failed to allocate buffer memory.");
        core_string_destroy(string_);
        return CORE_STRING_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(internal_data->buffer, buffer_size_);
    internal_data->buff_size = buffer_size_;
    return CORE_STRING_SUCCESS;
}

CORE_STRING_ERROR_CODE core_string_buffer_resize(uint64_t buffer_size_, core_string_t* const string_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_buffer_resize", "string_", string_);

    if(0 != string_->internal_data) {   // 既に内部データあり
        const uint64_t capacity = core_string_buffer_capacity(string_);
        if(capacity >= buffer_size_) {
            DEBUG_MESSAGE("core_string_buffer_resize - Requested buffer is already reserved. Requested size = %lld, Reserved size = %lld.", buffer_size_, capacity);
            return CORE_STRING_SUCCESS;
        }

        // 一時退避バッファを作成し、既存のデータをコピーする
        core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
        const uint64_t old_length = internal_data->length;
        char* tmp_buffer = core_malloc(old_length + 1);
        if(0 == tmp_buffer) {
            ERROR_MESSAGE("core_string_buffer_resize - Failed to allocate tmp_buffer memory.");
            return CORE_STRING_MEMORY_ALLOCATE_ERROR;
        }
        core_zero_memory(tmp_buffer, old_length + 1);
        if(0 != old_length) {
            if(!pfn_core_string_copy(internal_data->buffer, tmp_buffer, old_length + 1)) {
                core_free(tmp_buffer);
                ERROR_MESSAGE("core_string_buffer_resize - Failed to copy string.");
                return CORE_STRING_RUNTIME_ERROR;
            }
            core_free(internal_data->buffer);
        }
        // 既存バッファを一時的に削除し、再度メモリ確保を行う
        internal_data->buffer = core_malloc(buffer_size_);
        if(0 == internal_data->buffer) {
            core_free(tmp_buffer);
            ERROR_MESSAGE("core_string_buffer_resize - Failed to allocate new buffer memory.");
            return CORE_STRING_MEMORY_ALLOCATE_ERROR;
        }
        core_zero_memory(internal_data->buffer, buffer_size_);
        internal_data->buff_size = buffer_size_;

        // 一時退避バッファから新規バッファにデータを戻す
        if(!pfn_core_string_copy(tmp_buffer, internal_data->buffer, old_length + 1)) {
            core_free(tmp_buffer);
            ERROR_MESSAGE("core_string_buffer_resize - Failed to copy string.");
            return CORE_STRING_RUNTIME_ERROR;
        }
        core_free(tmp_buffer);
    } else {    // 内部データがまだない(完全に新規のメモリ確保)
        return core_string_buffer_reserve(buffer_size_, string_);
    }
    return CORE_STRING_SUCCESS;
}

uint64_t core_string_buffer_capacity(const core_string_t* const string_) {
    if(0 == string_) {
        DEBUG_MESSAGE("core_string_buffer_capacity - Argument string_ requires a valid pointer.");
        return INVALID_VALUE_U64;
    }
    if(0 == string_->internal_data) {
        return 0;
    }
    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
    return internal_data->buff_size;
}

bool core_string_is_empty(const core_string_t* const string_) {
    if(0 == string_) {
        WARN_MESSAGE("core_string_is_empty - Argument string_ requires a valid pointer.");
        return true;
    }
    if(0 == string_->internal_data) {
        return true;
    }
    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
    if(0 == internal_data->length) {
        return true;
    }
    return false;
}

bool core_string_equal(const core_string_t* const string1_, const core_string_t* const string2_) {
    if(0 == string1_ || 0 == string2_) {
        WARN_MESSAGE("core_string_equal - Arguments string1_ and string2_ requre valid pointers.");
        return false;
    }
    if(0 == string1_->internal_data) {
        WARN_MESSAGE("core_string_equal - Provided string1_ is not initialized.");
        return false;
    }
    if(0 == string2_->internal_data) {
        WARN_MESSAGE("core_string_equal - Provided string2_ is not initialized.");
        return false;
    }
    const core_string_internal_data_t* string1_internal_data = (core_string_internal_data_t*)(string1_->internal_data);
    const core_string_internal_data_t* string2_internal_data = (core_string_internal_data_t*)(string2_->internal_data);
    if(string1_internal_data->length != string2_internal_data->length) {
        return false;
    }
    for(uint64_t i = 0; i != string1_internal_data->length; ++i) {
        if(string1_internal_data->buffer[i] != string2_internal_data->buffer[i]) {
            return false;
        }
    }
    return true;
}

bool core_string_equal_from_char(const char* const str1_, const core_string_t* const string2_) {
    if(0 == str1_ || 0 == string2_) {
        WARN_MESSAGE("core_string_equal - Arguments string1_ and string2_ requre valid pointers.");
        return false;
    }
    if(0 == string2_->internal_data) {
        WARN_MESSAGE("core_string_equal - Provided string2_ is not initialized.");
        return false;
    }
    const uint64_t str1_len = pfn_string_length_from_char(str1_);
    const core_string_internal_data_t* string2_internal_data = (core_string_internal_data_t*)(string2_->internal_data);
    if(str1_len != string2_internal_data->length) {
        return false;
    }
    for(uint64_t i = 0; i != str1_len; ++i) {
        if(str1_[i] != string2_internal_data->buffer[i]) {
            return false;
        }
    }
    return true;
}

uint64_t core_string_length(const core_string_t* const string_) {
    if(0 == string_) {
        ERROR_MESSAGE("core_string_length - Argument string_ requres a valid pointer.");
        return INVALID_VALUE_U64;
    }
    if(0 == string_->internal_data) {
        ERROR_MESSAGE("core_string_length - Provided string_ is not initialized.");
        return 0;
    }
    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
    return internal_data->length;
}

const char* core_string_cstr(const core_string_t* const string_) {
    if(0 == string_) {
        ERROR_MESSAGE("core_string_cstr - Argument string_ requires a valid pointer.");
        return 0;
    }
    if(0 == string_->internal_data) {
        DEBUG_MESSAGE("core_string_cstr - Provided string_ is not initialized.");
        return 0;
    }
    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
    return internal_data->buffer;
}

CORE_STRING_ERROR_CODE core_string_concat(const core_string_t* const string_, core_string_t* const dst_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_concat", "string_", string_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_concat", "dst_", dst_);

    if(0 == string_->internal_data) {
        ERROR_MESSAGE("core_string_concat - Argument string_ is not initialized.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    const core_string_internal_data_t* string_internal_data = (core_string_internal_data_t*)(string_->internal_data);

    uint64_t required_buffer_size = 0;
    required_buffer_size += string_internal_data->length + 1;
    if(0 != dst_->internal_data) {
        const core_string_internal_data_t* dst_internal_data = (core_string_internal_data_t*)(dst_->internal_data);
        required_buffer_size += (dst_internal_data->length + 1);
    }

    const uint64_t dst_capacity = core_string_buffer_capacity(dst_);
    if(INVALID_VALUE_U64 == dst_capacity) {
        ERROR_MESSAGE("core_string_concat - Failed to get destination buffer capacity.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    if(required_buffer_size > dst_capacity) {
        if(CORE_STRING_SUCCESS != core_string_buffer_resize(required_buffer_size, dst_)) {
            ERROR_MESSAGE("core_string_concat - Failed to resize destination buffer.");
            return CORE_STRING_RUNTIME_ERROR;
        }
    }

    core_string_internal_data_t* dst_internal_data = (core_string_internal_data_t*)(dst_->internal_data);
    uint64_t dst_counter = dst_internal_data->length;
    for(uint64_t i = 0; i != string_internal_data->length; ++i) {
        dst_internal_data->buffer[dst_counter] = string_internal_data->buffer[i];
        dst_counter++;
    }
    dst_internal_data->buffer[dst_counter] = '\0';
    dst_internal_data->length = dst_counter;
    return CORE_STRING_SUCCESS;
}

CORE_STRING_ERROR_CODE core_string_substring_copy(const core_string_t* const src_, core_string_t* const dst_, uint16_t from_, uint16_t to_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_substring_copy", "src_", src_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_substring_copy", "dst_", dst_);
    if(from_ > to_) {
        ERROR_MESSAGE("core_string_substring_copy - Illegal argument. to_ must be larger than from_. [from_, to_] = [%d, %d].", from_, to_);
        return CORE_STRING_INVALID_ARGUMENT;
    }
    if(0 == src_->internal_data) {
        ERROR_MESSAGE("core_string_substring_copy - Argument src_ is not initialized.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    const core_string_internal_data_t* src_internal_data = (core_string_internal_data_t*)(src_->internal_data);
    if(to_ > src_internal_data->length) {
        ERROR_MESSAGE("core_string_substring_copy - Provided to_ is buffer range over.");
        return CORE_STRING_INVALID_ARGUMENT;
    }

    const uint64_t required_buffer_size = to_ - from_ + 2;
    const uint64_t dst_buffer_size = core_string_buffer_capacity(dst_);
    if(INVALID_VALUE_U64 == dst_buffer_size) {
        ERROR_MESSAGE("core_string_substring_copy - Failed to get destination buffer capacity.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    if(dst_buffer_size < required_buffer_size) {
        if(CORE_STRING_SUCCESS != core_string_buffer_resize(required_buffer_size, dst_)) {
            ERROR_MESSAGE("core_string_substring_copy - Failed to resize destination buffer.");
            return CORE_STRING_RUNTIME_ERROR;
        }
    }

    core_string_internal_data_t* dst_internal_data = (core_string_internal_data_t*)(dst_->internal_data);
    for(uint16_t i = from_, j = 0; i <= to_; ++i, ++j) {
        dst_internal_data->buffer[j] = src_internal_data->buffer[i];
    }
    dst_internal_data->buffer[to_ - from_ + 1] = '\0';
    dst_internal_data->length = to_ - from_ + 1;
    return CORE_STRING_SUCCESS;
}

CORE_STRING_ERROR_CODE core_string_trim(const core_string_t* const src_, core_string_t* const dst_, char ltrim_, char rtrim_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_trim", "src_", src_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_trim", "dst_", dst_);
    if(0 == src_->internal_data) {
        ERROR_MESSAGE("core_string_trim - Argument src_ is not initialized.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    const uint64_t src_length = core_string_length(src_);
    if(INVALID_VALUE_U64 == src_length) {
        ERROR_MESSAGE("core_string_trim - Failed to get src length.");
        return CORE_STRING_RUNTIME_ERROR;
    }

    uint64_t to = INVALID_VALUE_U64;
    core_string_internal_data_t* src_internal_data = (core_string_internal_data_t*)(src_->internal_data);
    for(int i = (int)(src_length - 1); i >= 0; --i) {
        if(src_internal_data->buffer[i] != rtrim_) {
            to = i;
            break;
        }
    }
    if (INVALID_VALUE_U64 == to) {
        // 空文字列を返す
        if(0 == dst_->internal_data) {
            core_string_buffer_reserve(2, dst_);
        } else {
            core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(dst_->internal_data);
            for(uint64_t i = 0; i != internal_data->length; ++i) {
                internal_data->buffer[i] = 0;
            }
        }
        return CORE_STRING_SUCCESS;
    }

    uint16_t from = 0;
    for(uint16_t i = 0; i != src_length; ++i) {
        if(src_internal_data->buffer[i] != ltrim_) {
            from = i;
            break;
        }
    }

    if (from > to) {
        // 空文字列を返す
        if(0 == dst_->internal_data) {
            core_string_buffer_reserve(2, dst_);
        } else {
            core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(dst_->internal_data);
            for(uint64_t i = 0; i != internal_data->length; ++i) {
                internal_data->buffer[i] = 0;
            }
        }
        return CORE_STRING_SUCCESS;
    } else {
        return core_string_substring_copy(src_, dst_, from, to);
    }
}

CORE_STRING_ERROR_CODE core_string_to_i32(const core_string_t* const string_, int32_t* out_value_) {
    CHECK_ARG_NULL_RETURN_ERROR("core_string_to_i32", "string_", string_);
    CHECK_ARG_NULL_RETURN_ERROR("core_string_to_i32", "out_value_", out_value_);
    if(0 == string_->internal_data) {
        ERROR_MESSAGE("core_string_to_i32 - Argument string_ is not initialized.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    core_string_internal_data_t* internal_data = (core_string_internal_data_t*)(string_->internal_data);
    if(0 == internal_data->length) {
        ERROR_MESSAGE("core_string_to_i32 - Provided string is empty.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    char* e = 0;
    long tmp = strtol(internal_data->buffer, &e, 10);
    if(*e != '\0') {
        ERROR_MESSAGE("core_string_to_i32 - Failed to convert string.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    if (tmp < INT32_MIN || tmp > INT32_MAX) {
        ERROR_MESSAGE("core_string_to_i32 - Value out of int32_t range.");
        return CORE_STRING_RUNTIME_ERROR;
    }
    *out_value_ = (int32_t)(tmp);
    return CORE_STRING_SUCCESS;
}

// 引数で与えた文字列の長さを取得する
static uint64_t pfn_string_length_from_char(const char* const str_) {
    if(0 == str_) {
        WARN_MESSAGE("pfn_string_length_from_char - Argument str_ is null.");
        return 0;
    }
    uint64_t len = 0;
    while('\0' != str_[len]) {
        len++;
    }
    return len;
}

// char型配列dst_にchar型配列src_の中身をコピーする
static bool pfn_core_string_copy(const char* const src_, char* const dst_, uint64_t dst_buff_size_) {
    core_zero_memory(dst_, dst_buff_size_);
    const uint64_t src_len = pfn_string_length_from_char(src_);
    if((src_len + 1) > dst_buff_size_) {
        ERROR_MESSAGE("pfn_core_string_copy - Buffer too small. src length: %llu, buffer size: %llu (must be > src length).", src_len, dst_buff_size_);
        return false;
    }
    for(uint64_t counter = 0; counter != src_len; ++counter) {
        dst_[counter] = src_[counter];
    }
    return true;
}
