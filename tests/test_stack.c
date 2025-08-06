#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdalign.h>

#include "include/test_stack.h"

#include "../containers/include/stack.h"

typedef struct simple_data_t {
    int32_t id;
    int32_t value;
} simple_data_t;

typedef struct padded_data_t {
    int8_t  type;
    int32_t value;
} padded_data_t;

typedef struct pointer_data_t {
    char* name;
    int32_t id;
} pointer_data_t;

static void test_stack_with_simple_data(void);
static void test_stack_with_padded_data(void);
static void test_stack_with_pointer_data(void);
static void test_stack_null_argument(void);

void test_stack(void) {
    test_stack_with_simple_data();
    test_stack_with_padded_data();
    test_stack_with_pointer_data();
    test_stack_null_argument();
}

static void test_stack_with_simple_data(void) {
    stack_t stack = STACK_INITIALIZER;

    // 構造体サイズとアライメントは通常4バイト
    const uint64_t max_count = 10;
    const STACK_ERROR_CODE result = stack_create(sizeof(simple_data_t), alignof(simple_data_t), max_count, &stack);
    assert(result == STACK_ERROR_CODE_SUCCESS);

    // push テスト
    for (int i = 0; i < 5; ++i) {
        simple_data_t data = { .id = i, .value = i * 10 };
        assert(stack_push(&stack, &data) == STACK_ERROR_CODE_SUCCESS);
    }

    // peek テスト
    const void* top_ptr = 0;
    assert(stack_pop_peek_ptr(&stack, (void**)(&top_ptr)) == STACK_ERROR_CODE_SUCCESS);
    const simple_data_t* top_data = (const simple_data_t*)top_ptr;
    assert(top_data->id == 4 && top_data->value == 40);  // 最後にpushしたデータ

    // discard テスト
    assert(stack_discard_top(&stack) == STACK_ERROR_CODE_SUCCESS);

    // pop テスト
    for (int i = 3; i >= 0; --i) {
        simple_data_t out;
        assert(stack_pop(&stack, &out) == STACK_ERROR_CODE_SUCCESS);
        assert(out.id == i && out.value == i * 10);
    }

    // 空状態チェック
    assert(stack_empty(&stack) == true);
    assert(stack_full(&stack) == false);

    // エラー処理チェック
    simple_data_t dummy;
    assert(stack_pop(&stack, &dummy) == STACK_ERROR_STACK_EMPTY);
    assert(stack_discard_top(&stack) == STACK_ERROR_STACK_EMPTY);

    stack_destroy(&stack);
}

static void test_stack_with_padded_data(void) {
    stack_t stack = STACK_INITIALIZER;

    const uint64_t max_count = 10;
    const STACK_ERROR_CODE result = stack_create(sizeof(padded_data_t), alignof(padded_data_t), max_count, &stack);
    assert(result == STACK_ERROR_CODE_SUCCESS);

    // push テスト
    for (int i = 0; i < 5; ++i) {
        padded_data_t data = { .type = (int8_t)(i + 1), .value = (i + 1) * 100 };
        assert(stack_push(&stack, &data) == STACK_ERROR_CODE_SUCCESS);
    }

    // peek テスト
    const void* top_ptr = NULL;
    assert(stack_pop_peek_ptr(&stack, &top_ptr) == STACK_ERROR_CODE_SUCCESS);
    const padded_data_t* top_data = (const padded_data_t*)top_ptr;
    assert(top_data->type == 5 && top_data->value == 500);

    // discard テスト
    assert(stack_discard_top(&stack) == STACK_ERROR_CODE_SUCCESS);

    // pop テスト
    for (int i = 4; i >= 1; --i) {
        padded_data_t out;
        assert(stack_pop(&stack, &out) == STACK_ERROR_CODE_SUCCESS);
        assert(out.type == i && out.value == i * 100);
    }

    // 空チェック
    assert(stack_empty(&stack) == true);
    stack_destroy(&stack);
}

static void test_stack_with_pointer_data(void) {
    stack_t stack = STACK_INITIALIZER;

    const uint64_t max_count = 10;
    const STACK_ERROR_CODE result = stack_create(sizeof(pointer_data_t), alignof(pointer_data_t), max_count, &stack);
    assert(result == STACK_ERROR_CODE_SUCCESS);

    // テストデータの用意（名前は固定文字列でOK）
    const char* names[] = { "Alice", "Bob", "Charlie", "Diana", "Eve" };

    // push
    for (int i = 0; i < 5; ++i) {
        pointer_data_t data = {
            .name = (char*)names[i],
            .id = i + 100
        };
        assert(stack_push(&stack, &data) == STACK_ERROR_CODE_SUCCESS);
    }

    // peek
    const void* peek_ptr = NULL;
    assert(stack_pop_peek_ptr(&stack, &peek_ptr) == STACK_ERROR_CODE_SUCCESS);
    const pointer_data_t* peek_data = (const pointer_data_t*)peek_ptr;
    assert(peek_data->id == 104);
    assert(strcmp(peek_data->name, "Eve") == 0);

    // discard
    assert(stack_discard_top(&stack) == STACK_ERROR_CODE_SUCCESS);

    // pop
    for (int i = 3; i >= 0; --i) {
        pointer_data_t out;
        assert(stack_pop(&stack, &out) == STACK_ERROR_CODE_SUCCESS);
        assert(out.id == i + 100);
        assert(strcmp(out.name, names[i]) == 0);
    }

    assert(stack_empty(&stack) == true);
    stack_destroy(&stack);
}

static void test_stack_null_argument(void) {
    stack_t stack = STACK_INITIALIZER;
    int dummy = 42;
    STACK_ERROR_CODE err;

    // stack_create
    err = stack_create(sizeof(int), alignof(int), 10, NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_create(0, alignof(int), 10, &stack); // element_size == 0
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_create(sizeof(int), 0, 10, &stack); // alignment == 0
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_create(sizeof(int), alignof(int), 0, &stack); // max count == 0
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_push
    err = stack_push(NULL, &dummy);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_push(&stack, NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_pop
    err = stack_pop(NULL, &dummy);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_pop(&stack, NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_pop_peek_ptr
    err = stack_pop_peek_ptr(NULL, (const void**)&dummy);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_pop_peek_ptr(&stack, NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_discard_top
    err = stack_discard_top(NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_clear
    err = stack_clear(NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_capacity
    uint64_t cap = 0;
    err = stack_capacity(NULL, &cap);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    err = stack_capacity(&stack, NULL);
    assert(err == STACK_ERROR_INVALID_ARGUMENT);

    // stack_full / stack_empty は bool返すので assert を活用
    assert(stack_full(NULL) == true);
    assert(stack_empty(NULL) == true);
}
