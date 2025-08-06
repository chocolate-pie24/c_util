#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#include "../include/stack.h"

#include "internal/stack_internal_data.h"

#include "../../core/include/message.h"
#include "../../core/include/core_memory.h"

#define CHECK_ARG_NULL_RETURN_ERROR(func_name_, arg_name_, ptr_) \
    if(0 == ptr_) { \
        ERROR_MESSAGE("%s - Argument %s requires a valid pointer.", func_name_, arg_name_); \
        return STACK_ERROR_INVALID_ARGUMENT; \
    } \

#define CHECK_ARG_NULL_RETURN_VOID(func_name_, arg_name_, ptr_) \
    if(0 == ptr_) { \
        WARN_MESSAGE("%s - Argument %s requires a valid pointer.", func_name_, arg_name_); \
        return; \
    } \

void stack_default_create(stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_VOID("stack_default_create", "stack_", stack_);
    stack_->internal_data = 0;
}

// この関数で一度確定したメモリープールサイズは変更不可
STACK_ERROR_CODE stack_create(uint64_t element_size_, uint8_t alignment_requirement_, uint64_t max_element_count_, stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_create", "stack_", stack_);
    if(0 == element_size_ || 0 == alignment_requirement_ || 0 == max_element_count_) {
        ERROR_MESSAGE("stack_create - Arguments element_size_ , alignment_requirement_ and max_element_count_ require non zero value.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    stack_destroy(stack_);
    stack_->internal_data = core_malloc(sizeof(stack_internal_data_t));
    if(0 == stack_->internal_data) {
        ERROR_MESSAGE("stack_create - Failed to allocate internal_data memory.");
        return STACK_ERROR_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(stack_->internal_data, sizeof(stack_t));
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    internal_data->alignment_requirement = alignment_requirement_;
    internal_data->element_size = element_size_;
    internal_data->max_element_count = max_element_count_;

    uint64_t diff = element_size_ % internal_data->alignment_requirement;   // アライメントのズレ量
    uint64_t padding_size = internal_data->alignment_requirement - diff;    // パディングサイズ
    padding_size = padding_size % internal_data->alignment_requirement;     // ピッタリの時のために計算
    internal_data->aligned_element_size = internal_data->element_size + padding_size;
    internal_data->buffer_capacity = internal_data->aligned_element_size * internal_data->max_element_count;
    internal_data->top_index = 0;

    internal_data->memory_pool = core_malloc(internal_data->buffer_capacity);
    if(0 == internal_data->memory_pool) {
        ERROR_MESSAGE("stack_create - Failed to allocate memory_pool memory.");
        return STACK_ERROR_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(internal_data->memory_pool, internal_data->buffer_capacity);

    return STACK_ERROR_CODE_SUCCESS;
}

void stack_destroy(stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_VOID("stack_destroy", "stack_", stack_);
    if(0 != stack_->internal_data) {
        stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
        core_free(internal_data->memory_pool);
        internal_data->memory_pool = 0;
    }
    core_free(stack_->internal_data);
    stack_->internal_data = 0;
}

STACK_ERROR_CODE stack_push(stack_t* const stack_, const void* const data_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_push", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_push", "data_", data_);
    if(0 == stack_->internal_data) {    // internal_dataが0の場合にstack_full=trueとなるため、この判定と下のstack_fullの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_push - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_full(stack_)) {
        ERROR_MESSAGE("stack_push - Provided stack is full.");
        return STACK_ERROR_STACK_FULL;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    char* dst = (char*)(internal_data->memory_pool + internal_data->top_index * internal_data->aligned_element_size);
    char* src = (char*)(data_);
    for(uint64_t i = 0; i != internal_data->aligned_element_size; ++i) {
        dst[i] = src[i];
    }
    internal_data->top_index++;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_pop(const stack_t* const stack_, void* const out_data_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop", "out_data_", out_data_);
    if(0 == stack_->internal_data) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_pop - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_pop - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    char* dst = (char*)(out_data_);
    char* src = (char*)(internal_data->memory_pool + (internal_data->top_index - 1) * internal_data->aligned_element_size);
    for(uint64_t i = 0; i != internal_data->aligned_element_size; ++i) {
        dst[i] = src[i];
    }
    internal_data->top_index--;
    return STACK_ERROR_CODE_SUCCESS;
}

// スタックのtopへの参照をconst void*として出力(スタックの中身は削除しない)
STACK_ERROR_CODE stack_pop_peek_ptr(const stack_t* const stack_, const void* *out_data_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop_peek_ptr", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop_peek_ptr", "out_data_", out_data_);
    if(0 == stack_->internal_data) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_pop_peek_ptr - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_pop_peek_ptr - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    *out_data_ = (const void*)(internal_data->memory_pool + (internal_data->top_index - 1) * internal_data->aligned_element_size);

    return STACK_ERROR_CODE_SUCCESS;
}

// スタックのtopのデータを破棄
STACK_ERROR_CODE stack_discard_top(const stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_discard_top", "stack_", stack_);
    if(0 == stack_->internal_data) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_discard_top - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_discard_top - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    internal_data->top_index--;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_clear(stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_clear", "stack_", stack_);
    if(0 == stack_->internal_data) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_clear - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    internal_data->top_index = 0;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_capacity(const stack_t* const stack_, uint64_t* const out_capacity_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_capacity", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_capacity", "out_capacity_", out_capacity_);
    if(0 == stack_->internal_data) {
        ERROR_MESSAGE("stack_capacity - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    *out_capacity_ = internal_data->buffer_capacity;
    return STACK_ERROR_CODE_SUCCESS;
}

// 与えられたスタックが未初期であればワーニングを出し、trueを返す
bool stack_full(const stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_full", "stack_", stack_);
    if(0 == stack_->internal_data) {
        WARN_MESSAGE("stack_full - Provided stack is not valid.");
        return true;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    if((internal_data->top_index + 1) >= internal_data->max_element_count) {
        return true;
    }
    return false;
}

// 与えられたスタックが未初期であればワーニングを出し、trueを返す
bool stack_empty(const stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_empty", "stack_", stack_);
    if(0 == stack_->internal_data) {
        WARN_MESSAGE("stack_empty - Provided stack is not valid.");
        return true;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    if(0 == internal_data->top_index) {
        return true;
    }
    return false;
}

const char* stack_error_code_to_string(STACK_ERROR_CODE err_code_) {
    switch(err_code_) {
        case STACK_ERROR_CODE_SUCCESS:
            return "stack error code: success";
        case STACK_ERROR_MEMORY_ALLOCATE_ERROR:
            return "stack error code: failed to allocate memory.";
        case STACK_ERROR_RUNTIME_ERROR:
            return "stack error code: runtime error.";
        case STACK_ERROR_INVALID_ARGUMENT:
            return "stack error code: invalid argument.";
        case STACK_ERROR_INVALID_STACK:
            return "stack error code: invalid stack.";
        case STACK_ERROR_STACK_EMPTY:
            return "stack error code: stack is empty.";
        case STACK_ERROR_STACK_FULL:
            return "stack error code: stack is full.";
        default:
            return "stack error code: undefined error.";
    }
}

void stack_debug_print(const stack_t* const stack_) {
    DEBUG_MESSAGE("stack_debug_print - Debug information for provided stack.");
    if(0 == stack_) {
        DEBUG_MESSAGE("\tArgument stack_ requires a valid pointer.");
        return;
    }
    if(0 == stack_->internal_data) {
        DEBUG_MESSAGE("\tProvided stack is not initialized.");
        return;
    }
    stack_internal_data_t* internal_data = (stack_internal_data_t*)(stack_->internal_data);
    DEBUG_MESSAGE("\telement_size          : %lld", internal_data->element_size);
    DEBUG_MESSAGE("\tbuffer_capacity(byte) : %lld", internal_data->buffer_capacity);
    DEBUG_MESSAGE("\tmax_element_count     : %lld", internal_data->max_element_count);
    DEBUG_MESSAGE("\taligned_element_size  : %lld", internal_data->aligned_element_size);
    DEBUG_MESSAGE("\ttop_index             : %lld", internal_data->top_index);
    DEBUG_MESSAGE("\talignment_requirement : %lld", internal_data->alignment_requirement);
}
