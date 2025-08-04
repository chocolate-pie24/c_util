#include "../include/dynamic_array.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#include "../../core/include/message.h"
#include "../../core/include/core_memory.h"

#include "internal/dynamic_array_internal_data.h"

/**
 * @brief 引数のNULLチェックを行い、NULLであればCORE_STRING_INVALID_ARGUMENTで処理を終了するマクロ
 *
 */
#define CHECK_ARG_NULL_RETURN_ERROR(func_name_, arg_name_, ptr_) \
    if(0 == ptr_) { \
        ERROR_MESSAGE("%s - Argument %s requires a valid pointer.", func_name_, arg_name_); \
        return DYNAMIC_ARRAY_INVALID_ARGUMENT; \
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

void dynamic_array_default_create(dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_VOID("dynamic_array_default_create", "dynamic_array_", dynamic_array_);
    dynamic_array_->internal_data = 0;
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_create(uint64_t element_size_, uint8_t alignment_requirement_, uint64_t max_element_count_, dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_create", "dynamic_array_", dynamic_array_);
    if(0 == element_size_ || 0 == alignment_requirement_) {
        ERROR_MESSAGE("dynamic_array_create - Arguments element_size_ and alignment_requirement_ require non zero value.");
        return DYNAMIC_ARRAY_INVALID_ARGUMENT;
    }
    dynamic_array_destroy(dynamic_array_);
    dynamic_array_->internal_data = core_malloc(sizeof(dynamic_array_internal_data_t));
    if(0 == dynamic_array_->internal_data) {
        ERROR_MESSAGE("dynamic_array_create - Failed to allocate internal_data memory.");
        return DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(dynamic_array_->internal_data, sizeof(dynamic_array_internal_data_t));
    dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
    internal_data->alignment_requirement = alignment_requirement_;
    internal_data->element_size = element_size_;
    internal_data->max_element_count = max_element_count_;

    uint64_t diff = element_size_ % internal_data->alignment_requirement;   // アライメントのズレ量
    uint64_t padding_size = internal_data->alignment_requirement - diff;    // パディングサイズ
    padding_size = padding_size % internal_data->alignment_requirement;     // ピッタリの時のために計算
    internal_data->aligned_element_size = internal_data->element_size + padding_size;

    return dynamic_array_reserve(max_element_count_, dynamic_array_);
}

void dynamic_array_destroy(dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_VOID("dynamic_array_destroy", "dynamic_array_", dynamic_array_);
    if(0 != dynamic_array_->internal_data) {
        dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
        core_free(internal_data->memory_pool);
        internal_data->memory_pool = 0;
    }
    core_free(dynamic_array_->internal_data);
    dynamic_array_->internal_data = 0;
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_reserve(uint64_t max_element_count_, dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_reserve", "dynamic_array_", dynamic_array_);
    if(0 == max_element_count_) {
        WARN_MESSAGE("dynamic_array_reserve - Argument max_element_count_ is 0. Nothing to be done.");
        return DYNAMIC_ARRAY_SUCCESS;
    }
    if(0 != dynamic_array_->internal_data) {
        dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
        core_free(internal_data->memory_pool);
        internal_data->memory_pool = 0;
        internal_data->buffer_capacity = 0;
        internal_data->element_count = 0;
    }
    dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
    internal_data->memory_pool = core_malloc(internal_data->max_element_count * internal_data->aligned_element_size);
    if(0 == internal_data->memory_pool) {
        ERROR_MESSAGE("dynamic_array_reserve - Failed to allocate memory_pool memory.");
        return DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(internal_data->memory_pool, internal_data->max_element_count * internal_data->aligned_element_size);
    internal_data->buffer_capacity = internal_data->max_element_count * internal_data->aligned_element_size;
    return DYNAMIC_ARRAY_SUCCESS;
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_resize(uint64_t max_element_count_, dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_resize", "dynamic_array_", dynamic_array_);
    if(0 == max_element_count_) {
        WARN_MESSAGE("dynamic_array_resize - Argument max_element_count_ is 0. Nothing to be done.");
        return DYNAMIC_ARRAY_SUCCESS;
    }
    if(0 == dynamic_array_->internal_data) {
        return dynamic_array_reserve(max_element_count_, dynamic_array_);
    }
    dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
    if (max_element_count_ < internal_data->element_count) {
        ERROR_MESSAGE("dynamic_array_resize - Cannot resize to smaller max_element_count than current element_count.");
        return DYNAMIC_ARRAY_INVALID_ARGUMENT;
    }
    char* buffer = core_malloc(internal_data->buffer_capacity);
    if(0 == buffer) {
        ERROR_MESSAGE("dynamic_array_resize - Failed to allocate swap memory.");
        return DYNAMIC_ARRAY_MEMORY_ALLOCATE_ERROR;
    }
    char* src_ptr = (char*)(internal_data->memory_pool);
    const uint64_t copy_size = internal_data->element_count * internal_data->aligned_element_size;
    for(uint64_t i = 0; i != copy_size; ++i) {
        buffer[i] = src_ptr[i];
    }
    core_free(internal_data->memory_pool);
    internal_data->memory_pool = 0;
    const uint64_t escape_count = internal_data->element_count;
    DYNAMIC_ARRAY_ERROR_CODE result_reserve = dynamic_array_reserve(max_element_count_, dynamic_array_);
    if(DYNAMIC_ARRAY_SUCCESS != result_reserve) {
        ERROR_MESSAGE("dynamic_array_resize - Failed to reserve memory_pool.");
        core_free(buffer);
        return result_reserve;
    }
    char* dst_ptr = (char*)(internal_data->memory_pool);
    for(uint64_t i = 0; i != internal_data->buffer_capacity; ++i) {
        dst_ptr[i] = buffer[i];
    }
    core_free(buffer);
    internal_data->element_count = escape_count;
    internal_data->max_element_count = max_element_count_;
    return DYNAMIC_ARRAY_SUCCESS;
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_capacity(const dynamic_array_t* const dynamic_array_, uint64_t* const out_capacity_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_capacity", "dynamic_array_", dynamic_array_);
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_capacity", "out_capacity_", out_capacity_);
    if(0 == dynamic_array_->internal_data) {
        ERROR_MESSAGE("dynamic_array_capacity - Provided dynamic_array_ is not initialized. Call dynamic_array_create.");
        return DYNAMIC_ARRAY_INVALID_DARRAY;
    } else {
        dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
        *out_capacity_ = internal_data->buffer_capacity / internal_data->aligned_element_size;
        return DYNAMIC_ARRAY_SUCCESS;
    }
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_size(const dynamic_array_t* const dynamic_array_, uint64_t* const out_size_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_size", "dynamic_array_", dynamic_array_);
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_size", "out_size_", out_size_);
    if(0 == dynamic_array_->internal_data) {
        ERROR_MESSAGE("dynamic_array_size - Provided dynamic_array_ is not initialized. Call dynamic_array_create.");
        return DYNAMIC_ARRAY_INVALID_DARRAY;
    } else {
        dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
        *out_size_ = internal_data->element_count;
        return DYNAMIC_ARRAY_SUCCESS;
    }
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_push(const void* const object_, dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_element_push", "dynamic_array_", dynamic_array_);
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_element_push", "object_", object_);
    if(0 == dynamic_array_->internal_data) {
        ERROR_MESSAGE("dynamic_array_element_push - Provided dynamic_array_ is not initialized. Call dynamic_array_create.");
        return DYNAMIC_ARRAY_INVALID_DARRAY;
    } else {
        dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
        if(internal_data->element_count == internal_data->max_element_count) {
            ERROR_MESSAGE("dynamic_array_element_push - Dynamic array buffer full.");
            return DYNAMIC_ARRAY_BUFFER_FULL;
        }
        char* dst_ptr = (char*)(internal_data->memory_pool + (internal_data->aligned_element_size * internal_data->element_count));
        char* src_ptr = (char*)object_;
        for(uint64_t i = 0; i != internal_data->element_size; ++i) {
            dst_ptr[i] = src_ptr[i];
        }
        internal_data->element_count++;
    }
    return DYNAMIC_ARRAY_SUCCESS;
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_ref(uint64_t element_index_, const dynamic_array_t* const dynamic_array_, void* const out_object_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_element_ref", "dynamic_array_", dynamic_array_);
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_element_ref", "out_object_", out_object_);
    if(0 == dynamic_array_->internal_data) {
        ERROR_MESSAGE("dynamic_array_element_ref - Provided dynamic_array_ is not initialized.");
        return DYNAMIC_ARRAY_INVALID_DARRAY;
    }
    dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
    if(element_index_ >= internal_data->element_count) {
        ERROR_MESSAGE("dynamic_array_element_ref - Requested element_index_ is out of range.");
        return DYNAMIC_ARRAY_OUT_OF_RANGE;
    }
    char* base = (char*)(internal_data->memory_pool);
    char* src_ptr = base + (internal_data->aligned_element_size * element_index_);
    char* dst_ptr = (char*)(out_object_);
    for(uint64_t i = 0; i != internal_data->aligned_element_size; ++i) {
        dst_ptr[i] = src_ptr[i];
    }
    return DYNAMIC_ARRAY_SUCCESS;
}

DYNAMIC_ARRAY_ERROR_CODE dynamic_array_element_set(uint64_t element_index_, void* object_, dynamic_array_t* const dynamic_array_) {
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_element_set", "dynamic_array_", dynamic_array_);
    CHECK_ARG_NULL_RETURN_ERROR("dynamic_array_element_set", "object_", object_);
    if(0 == dynamic_array_->internal_data) {
        ERROR_MESSAGE("dynamic_array_element_set - Provided dynamic_array_ is not initialized.");
        return DYNAMIC_ARRAY_INVALID_DARRAY;
    }
    dynamic_array_internal_data_t* internal_data = (dynamic_array_internal_data_t*)(dynamic_array_->internal_data);
    if(element_index_ >= internal_data->element_count) {
        ERROR_MESSAGE("dynamic_array_element_set - Requested element_index_ is out of range.");
        return DYNAMIC_ARRAY_OUT_OF_RANGE;
    }

    char* src_ptr = (char*)(object_);
    char* dst_ptr = internal_data->memory_pool + (internal_data->aligned_element_size * element_index_);
    for(uint64_t i = 0; i != internal_data->element_size; ++i) {
        dst_ptr[i] = src_ptr[i];
    }
    return DYNAMIC_ARRAY_SUCCESS;
}
