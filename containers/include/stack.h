// stackオブジェクトコンセプト
// - 当面は最低限の機能のみでシンプルなオブジェクトを作成する。必要に応じて機能追加予定
// - stack_createで作成したバッファサイズは変更されず、拡張はしない
// - 拡張を要する場合にはdynamic_arrayを使用する
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum STACK_ERROR_CODE {
    STACK_ERROR_CODE_SUCCESS = 0x00,
    STACK_ERROR_MEMORY_ALLOCATE_ERROR = 0x01,
    STACK_ERROR_RUNTIME_ERROR = 0x02,
    STACK_ERROR_INVALID_ARGUMENT = 0x03,
    STACK_ERROR_INVALID_STACK = 0x04,
    STACK_ERROR_STACK_EMPTY = 0x05,
    STACK_ERROR_STACK_FULL = 0x06,
} STACK_ERROR_CODE;

typedef struct stack_t {
    void* internal_data;
} stack_t;

#define STACK_INITIALIZER { 0 }

void stack_default_create(stack_t* const stack_);

STACK_ERROR_CODE stack_create(uint64_t element_size_, uint8_t alignment_requirement_, uint64_t max_element_count_, stack_t* const stack_);

void stack_destroy(stack_t* const stack_);

STACK_ERROR_CODE stack_push(stack_t* const stack_, const void* const data_);

// スタックのtopをout_data_にコピーし、スタックのtopを削除
STACK_ERROR_CODE stack_pop(const stack_t* const stack_, void* const out_data_);

// スタックのtopへの参照をconst void*として出力(スタックの中身は削除しない)
STACK_ERROR_CODE stack_pop_peek_ptr(const stack_t* const stack_, const void* *out_data_);

// スタックのtopのデータを破棄
STACK_ERROR_CODE stack_discard_top(const stack_t* const stack_);

// これは危険なので禁止
// STACK_ERROR_CODE stack_pop_ptr(const stack_t* const stack_, void** out_data_);

STACK_ERROR_CODE stack_clear(stack_t* const stack_);

STACK_ERROR_CODE stack_capacity(const stack_t* const stack_, uint64_t* const out_capacity_);

bool stack_full(const stack_t* const stack_);

bool stack_empty(const stack_t* const stack_);

const char* stack_error_code_to_string(STACK_ERROR_CODE err_code_);

void stack_debug_print(const stack_t* const stack_);
