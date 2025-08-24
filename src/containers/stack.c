/**
 * @file stack.c
 * @author chocolate-pie24
 * @brief スタックオブジェクト(stack_t)用API関数の実装ファイル
 *
 * @version 0.1
 * @date 2025-07-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "containers/stack.h"

#include "internal/stack_internal_data.h"

#include "core/message.h"
#include "core/core_memory.h"

typedef enum FLAG_BIT_POSITION {
    FLAG_BIT_ELEMENT_SIZE = 0x00,
    FLAG_BIT_MAX_ELEMENT_COUNT = 0x01,
    FLAG_BIT_ALIGNMENT_REQUIREMENT = 0x02,
    FLAG_BIT_MAX = 0x03,
} FLAG_BIT_POSITION;

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

static bool valid_stack(const stack_t* const stack_);
static void flag_set(FLAG_BIT_POSITION bit_pos_, bool should_on_, uint8_t* dst_);
static bool flag_get(FLAG_BIT_POSITION bit_pos_, uint8_t flags_);
static bool is_power_of_two(size_t val_);

// TODO: memory_poolの先頭アドレスのアライメント保証
STACK_ERROR_CODE stack_create(size_t element_size_, size_t alignment_requirement_, size_t max_element_count_, struct stack_t** stack_) {
    if(0 == stack_) {           // stack_tオブジェクトのポインタ変数のアドレスがNULL -> オブジェクトインスタンス生成不可でエラー
        ERROR_MESSAGE("stack_create - stack_ must be non-NULL.");
        return STACK_ERROR_INVALID_ARGUMENT;
    } else if(0 != *stack_) {   // *stack_tがNULLではない -> このオブジェクトはすでに実体を持っているためエラー
        ERROR_MESSAGE("stack_create - *stack_ must be NULL.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    if(0 == element_size_ || 0 == alignment_requirement_ || 0 == max_element_count_) {
        ERROR_MESSAGE("stack_create - Arguments element_size_, alignment_requirement_ and max_element_count_ require non zero value.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    if(!is_power_of_two(alignment_requirement_)) {
        ERROR_MESSAGE("stack_create - Argument alignment_requirement_ must be a power of two.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }

    stack_t* stack = core_malloc(sizeof(struct stack_t));
    if(0 == stack) {
        ERROR_MESSAGE("stack_create - Failed to allocate stack object memory.");
        return STACK_ERROR_MEMORY_ALLOCATE_ERROR;
    }
    const size_t diff = element_size_ % alignment_requirement_;                             // アライメントのズレ量
    const size_t padding_size = (alignment_requirement_ - diff) % alignment_requirement_;   // パディングサイズ+ぴったりの時のために%
    if(padding_size > (SIZE_MAX - element_size_)) {
        ERROR_MESSAGE("stack_create - Provided element_size_ is too big.");
        core_free(stack);
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    const size_t aligned_element_size = element_size_ + padding_size;
    if(aligned_element_size > (SIZE_MAX / max_element_count_)) {
        ERROR_MESSAGE("stack_create - Provided max_element_count_ is too big.");
        core_free(stack);
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    const size_t buffer_size = aligned_element_size * max_element_count_;
    stack->memory_pool = core_malloc(buffer_size);
    if(0 == stack->memory_pool) {
        ERROR_MESSAGE("stack_create - Failed to allocate memory_pool memory.");
        core_free(stack);
        return STACK_ERROR_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(stack->memory_pool, buffer_size);

    stack->valid_flags = 0;
    stack->alignment_requirement = alignment_requirement_;
    flag_set(FLAG_BIT_ALIGNMENT_REQUIREMENT, true, &stack->valid_flags);

    stack->element_size = element_size_;
    flag_set(FLAG_BIT_ELEMENT_SIZE, true, &stack->valid_flags);

    stack->max_element_count = max_element_count_;
    flag_set(FLAG_BIT_MAX_ELEMENT_COUNT, true, &stack->valid_flags);

    stack->aligned_element_size = aligned_element_size;
    stack->buffer_size = buffer_size;
    stack->top_index = 0;

    *stack_ = stack;

    return STACK_ERROR_CODE_SUCCESS;
}

void stack_destroy(struct stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_VOID("stack_destroy", "stack_", stack_);
    if(0 != stack_->memory_pool) {
        core_free(stack_->memory_pool);
        stack_->memory_pool = 0;
    }
    stack_default_create(stack_);
}

STACK_ERROR_CODE stack_reserve(uint64_t max_element_count_, struct stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_reserve", "stack_", stack_);
    if(0 == max_element_count_) {
        ERROR_MESSAGE("stack_reserve - Argument max_element_count_ requires a non-zero value.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    if(!valid_stack(stack_)) {
        ERROR_MESSAGE("stack_reserve - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_->aligned_element_size > (UINT64_MAX / max_element_count_)) {
        ERROR_MESSAGE("stack_reserve - Provided max_element_count_ is too big.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }

    const uint64_t new_buffer_size = stack_->aligned_element_size * max_element_count_;
    void* new_buffer = core_malloc(new_buffer_size);
    if(0 == new_buffer) {
        ERROR_MESSAGE("stack_reserve - Failed to allocate new buffer memory.");
        return STACK_ERROR_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(new_buffer, new_buffer_size);

    void* old_buffer_ptr = stack_->memory_pool;
    stack_->memory_pool = new_buffer;
    stack_->max_element_count = max_element_count_;
    stack_->buffer_size = stack_->aligned_element_size * max_element_count_;
    stack_->top_index = 0;

    core_free((void*)old_buffer_ptr);
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_resize(uint64_t max_element_count_, struct stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_resize", "stack_", stack_);
    if(0 == max_element_count_) {
        ERROR_MESSAGE("stack_resize - Argument max_element_count_ requires a non-zero value.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    if(!valid_stack(stack_)) {
        ERROR_MESSAGE("stack_resize - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(max_element_count_ <= stack_->max_element_count) {
        ERROR_MESSAGE("stack_resize - Shrinking the buffer is not allowed.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }
    if(stack_->aligned_element_size > (UINT64_MAX / max_element_count_)) {
        ERROR_MESSAGE("stack_resize - Provided max_element_count_ is too big.");
        return STACK_ERROR_INVALID_ARGUMENT;
    }

    // データコピートランザクション
    //  新領域の確保とデータのコピーに失敗した際に元の状態に戻せるようにするため、
    //  新領域確保 -> データコピー -> ポインタ差し替え -> 旧領域削除
    //  の手順を踏む

    // 新バッファメモリ確保
    const uint64_t new_buffer_size = stack_->aligned_element_size * max_element_count_;
    void* new_buffer = core_malloc(new_buffer_size);
    if(0 == new_buffer) {
        ERROR_MESSAGE("stack_resize - Failed to allocate new buffer memory.");
        return STACK_ERROR_MEMORY_ALLOCATE_ERROR;
    }
    core_zero_memory(new_buffer, new_buffer_size);

    // 旧バッファからデータコピー
    char* old_buffer_ptr = (char*)(stack_->memory_pool);
    char* new_buffer_ptr = (char*)(new_buffer);
    for(uint64_t i = 0; i != (stack_->aligned_element_size * stack_->top_index); ++i) {
        new_buffer_ptr[i] = old_buffer_ptr[i];
    }

    // ポインタ差し替え
    stack_->memory_pool = new_buffer;

    stack_->max_element_count = max_element_count_;
    stack_->buffer_size = new_buffer_size;

    core_free((void*)(old_buffer_ptr));
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_push(struct stack_t* const stack_, const void* const data_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_push", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_push", "data_", data_);
    if(!valid_stack(stack_)) {    // internal_dataが0の場合にstack_full=trueとなるため、この判定と下のstack_fullの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_push - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_full(stack_)) {
        ERROR_MESSAGE("stack_push - Provided stack is full.");
        return STACK_ERROR_STACK_FULL;
    }
    char* base = (char*)(stack_->memory_pool);
    char* dst = base + (stack_->top_index * stack_->aligned_element_size);
    char* src = (char*)(data_);
    core_zero_memory((void*)dst, stack_->aligned_element_size);  // この後のコピーではelement_size分しかコピーされないため、パディング領域が未初期化になる。
                                                                        // この場合、resizeでバッファをコピーする際に未初期化領域にアクセスすることになり、valgrind等でワーニングが出る
    for(uint64_t i = 0; i != stack_->element_size; ++i) {
        dst[i] = src[i];
    }
    stack_->top_index++;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_pop(struct stack_t* const stack_, void* const out_data_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop", "out_data_", out_data_);
    if(!valid_stack(stack_)) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_pop - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_pop - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    char* dst = (char*)(out_data_);
    char* base = (char*)(stack_->memory_pool);
    char* src = base + ((stack_->top_index - 1) * stack_->aligned_element_size);
    for(uint64_t i = 0; i != stack_->element_size; ++i) {
        dst[i] = src[i];
    }
    stack_->top_index--;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_pop_peek_ptr(const struct stack_t* const stack_, const void* *out_data_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop_peek_ptr", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_pop_peek_ptr", "out_data_", out_data_);
    if(!valid_stack(stack_)) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_pop_peek_ptr - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_pop_peek_ptr - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    char* base = (char*)(stack_->memory_pool);
    *out_data_ = (const void*)(base + (stack_->top_index - 1) * stack_->aligned_element_size);

    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_discard_top(struct stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_discard_top", "stack_", stack_);
    if(!valid_stack(stack_)) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_discard_top - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    if(stack_empty(stack_)) {
        ERROR_MESSAGE("stack_discard_top - Provided stack is empty.");
        return STACK_ERROR_STACK_EMPTY;
    }
    stack_->top_index--;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_clear(struct stack_t* const stack_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_clear", "stack_", stack_);
    if(!valid_stack(stack_)) {    // internal_dataが0の場合にstack_empty=trueとなるため、この判定と下のstack_emptyの判定を入れ替えないこと！！
        ERROR_MESSAGE("stack_clear - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    stack_->top_index = 0;
    return STACK_ERROR_CODE_SUCCESS;
}

STACK_ERROR_CODE stack_capacity(const struct stack_t* const stack_, uint64_t* const out_capacity_) {
    CHECK_ARG_NULL_RETURN_ERROR("stack_capacity", "stack_", stack_);
    CHECK_ARG_NULL_RETURN_ERROR("stack_capacity", "out_capacity_", out_capacity_);
    if(!valid_stack(stack_)) {
        ERROR_MESSAGE("stack_capacity - Provided stack is not valid.");
        return STACK_ERROR_INVALID_STACK;
    }
    *out_capacity_ = stack_->max_element_count;
    return STACK_ERROR_CODE_SUCCESS;
}

bool stack_full(const struct stack_t* const stack_) {
    if(!valid_stack(stack_)) {
        WARN_MESSAGE("stack_full - Provided stack is not valid.");
        return true;
    }
    if(stack_->top_index >= stack_->max_element_count) {
        return true;
    }
    return false;
}

bool stack_empty(const struct stack_t* const stack_) {
    if(!valid_stack(stack_)) {
        WARN_MESSAGE("stack_empty - Provided stack is not valid.");
        return true;
    }
    if(0 == stack_->top_index) {
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

void stack_debug_print(const struct stack_t* const stack_) {
    DEBUG_MESSAGE("stack_debug_print - Debug information for provided stack.");
    if(0 == stack_) {
        DEBUG_MESSAGE("\tArgument stack_ requires a valid pointer.");
        return;
    }
    DEBUG_MESSAGE("\telement_size          : %zu", stack_->element_size);
    DEBUG_MESSAGE("\tbuffer_size(byte)     : %zu", stack_->buffer_size);
    DEBUG_MESSAGE("\tmax_element_count     : %zu", stack_->max_element_count);
    DEBUG_MESSAGE("\taligned_element_size  : %zu", stack_->aligned_element_size);
    DEBUG_MESSAGE("\ttop_index             : %zu", stack_->top_index);
    DEBUG_MESSAGE("\talignment_requirement : %zu", stack_->alignment_requirement);
}

static bool valid_stack(const struct stack_t* const stack_) {
    if(0 == stack_) {
        WARN_MESSAGE("valid_stack - Argument stack_ requires a valid pointer.");
        return false;
    }
    const bool was_provided_element_size = flag_get(FLAG_BIT_ELEMENT_SIZE, stack_->valid_flags);
    const bool was_provided_max_element_count = flag_get(FLAG_BIT_MAX_ELEMENT_COUNT, stack_->valid_flags);
    const bool was_provided_alignment_requirement = flag_get(FLAG_BIT_ALIGNMENT_REQUIREMENT, stack_->valid_flags);
    if(!was_provided_element_size) {
        DEBUG_MESSAGE("valid_stack - Provided stack is not valid. Element size is not provided.");
    }
    if(!was_provided_max_element_count) {
        DEBUG_MESSAGE("valid_stack - Provided stack is not valid. Max element count is not provided.");
    }
    if(!was_provided_alignment_requirement) {
        DEBUG_MESSAGE("valid_stack - Provided stack is not valid. Alignment requirement is not provided.");
    }
    if(was_provided_element_size && was_provided_max_element_count && was_provided_alignment_requirement) {
        return true;
    }
    return false;
}

static void flag_set(FLAG_BIT_POSITION bit_pos_, bool should_on_, uint8_t* dst_) {
    if(0 == dst_) {
        ERROR_MESSAGE("flag_set - Argument dst_ requires a valid pointer.");
        return;
    }
    if(bit_pos_ >= FLAG_BIT_MAX) {
        ERROR_MESSAGE("flag_set - Provided bit position is not valid.");
        return;
    }
    if(should_on_) {
        *dst_ |= (0x01 << bit_pos_);
    } else {
        *dst_ &= ~(0x01 << bit_pos_);
    }
}

static bool flag_get(FLAG_BIT_POSITION bit_pos_, uint8_t flags_) {
    if (bit_pos_ >= FLAG_BIT_MAX) {
        ERROR_MESSAGE("flag_get - Provided bit position is not valid.");
        return false;
    }
    return (flags_ & (0x01 << bit_pos_)) != 0;
}

// 引数val_が2の冪乗かを判定する
static bool is_power_of_two(size_t val_) {
    return (0 != val_) && (0 == (val_ & (val_ - 1)));
}
