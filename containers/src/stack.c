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
    if(stack_full(stack_)) {
        ERROR_MESSAGE("stack_push - Provided stack is full.");
        return STACK_ERROR_BUFFER_FULL;
    }
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_pop(const stack_t* const stack_, void* const out_data_) {
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_pop - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_clear(stack_t* const stack_) {
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_capacity(const stack_t* const stack_, uint64_t* const out_capacity_) {
    return STACK_ERROR_CODE_SUCCESS;
}

bool stack_full(const stack_t* const stack_) {
    return false;
}

bool stack_empty(const stack_t* const stack_) {
    return false;
}
