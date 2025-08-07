#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdalign.h>

#include "../include/stack.h"
#include "test_stack.h"

// パディング不要構造体
typedef struct {
    int id;
} TestStructPlain;

// パディングあり構造体（alignment 16）
typedef struct {
    char c;
    double d;
} TestStructPadded;

// ポインタを含む構造体
typedef struct {
    int* ptr;
    int value;
} TestStructPointer;

#define STACK_SIZE 10

static void test_stack_invalid_usage(void);
static void test_stack_basic_operations(void);
static void test_stack_resize(void);
static void test_stack_reserve(void);
static void test_stack_empty_and_full(void);

void test_stack(void) {
    test_stack_basic_operations();
    test_stack_invalid_usage();
    test_stack_resize();
    test_stack_reserve();
    test_stack_empty_and_full();
}

static void test_stack_basic_operations(void) {
    {
        stack_t s = STACK_INITIALIZER;
        stack_create(sizeof(TestStructPlain), alignof(TestStructPlain), STACK_SIZE, &s);
        for (int i = 0; i < STACK_SIZE; ++i) {
            TestStructPlain data = { .id = i };
            assert(stack_push(&s, &data) == STACK_ERROR_CODE_SUCCESS);
        }
        for (int i = STACK_SIZE - 1; i >= 0; --i) {
            TestStructPlain out;
            assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
            assert(out.id == i);
        }
        stack_destroy(&s);
    }
    {
        stack_t s = STACK_INITIALIZER;
        stack_create(sizeof(TestStructPadded), alignof(TestStructPadded), STACK_SIZE, &s);
        for (int i = 0; i < STACK_SIZE; ++i) {
            TestStructPadded data = { .c = (char)(i + 65), .d = (double)i * 1.5 };
            assert(stack_push(&s, &data) == STACK_ERROR_CODE_SUCCESS);
        }
        for (int i = STACK_SIZE - 1; i >= 0; --i) {
            TestStructPadded out;
            assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
            assert(out.c == (char)(i + 65));
            assert(out.d == (double)i * 1.5);
        }
        stack_destroy(&s);
    }
    {
        stack_t s = STACK_INITIALIZER;
        stack_create(sizeof(TestStructPointer), alignof(TestStructPointer), STACK_SIZE, &s);
        for (int i = 0; i < STACK_SIZE; ++i) {
            static int buffer[STACK_SIZE];
            TestStructPointer data = { .ptr = &buffer[i], .value = i * 10 };
            assert(stack_push(&s, &data) == STACK_ERROR_CODE_SUCCESS);
        }
        for (int i = STACK_SIZE - 1; i >= 0; --i) {
            TestStructPointer out;
            assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
            assert(out.value == i * 10);
        }
        stack_destroy(&s);
    }
}

static void test_stack_invalid_usage(void) {
    stack_t s = STACK_INITIALIZER;
    stack_default_create(&s);
    TestStructPlain dummy = { .id = 1 };
    TestStructPlain out;

    assert(stack_push(NULL, &dummy) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_push(&s, NULL) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_push(&s, &dummy) == STACK_ERROR_INVALID_STACK);

    assert(stack_pop(NULL, &out) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_pop(&s, NULL) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_pop(&s, &out) == STACK_ERROR_INVALID_STACK);

    assert(stack_discard_top(NULL) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_discard_top(&s) == STACK_ERROR_INVALID_STACK);

    assert(stack_clear(NULL) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_clear(&s) == STACK_ERROR_INVALID_STACK);

    assert(stack_capacity(NULL, NULL) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_capacity(&s, NULL) == STACK_ERROR_INVALID_ARGUMENT);
    assert(stack_capacity(&s, &(uint64_t){0}) == STACK_ERROR_INVALID_STACK);

    stack_destroy(&s); // safe even if not created
}

static void test_stack_resize(void) {
    stack_t s = STACK_INITIALIZER;
    assert(stack_create(sizeof(int), alignof(int), 5, &s) == STACK_ERROR_CODE_SUCCESS);

    // pushで3つ入れる
    for (int i = 0; i < 3; ++i) {
        assert(stack_push(&s, &i) == STACK_ERROR_CODE_SUCCESS);
    }

    // resizeで10に拡張（縮小は禁止）
    assert(stack_resize(10, &s) == STACK_ERROR_CODE_SUCCESS);

    // 追加 push (6個目まで入る)
    for (int i = 3; i < 10; ++i) {
        assert(stack_push(&s, &i) == STACK_ERROR_CODE_SUCCESS);
    }

    // 10個入れたら full
    assert(stack_full(&s) == true);

    // popして値を確認（LIFO）
    for (int i = 9; i >= 0; --i) {
        int out;
        assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
        assert(out == i);
    }

    stack_destroy(&s);

    // 異常系: NULLポインタ
    assert(stack_resize(10, NULL) == STACK_ERROR_INVALID_ARGUMENT);

    // 異常系: 無効なstack
    stack_t s_invalid = {0};
    assert(stack_resize(10, &s_invalid) == STACK_ERROR_INVALID_STACK);

    // 異常系: max_element_count_ == 0
    assert(stack_create(sizeof(int), alignof(int), 5, &s) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_resize(0, &s) == STACK_ERROR_INVALID_ARGUMENT);
    stack_destroy(&s);
}

static void test_stack_reserve(void) {
    stack_t s = STACK_INITIALIZER;
    assert(stack_create(sizeof(int), alignof(int), 5, &s) == STACK_ERROR_CODE_SUCCESS);

    // 初期push
    for (int i = 0; i < 5; ++i) {
        assert(stack_push(&s, &i) == STACK_ERROR_CODE_SUCCESS);
    }

    // reserveで上書き（バッファは破棄されるので空になる）
    assert(stack_reserve(8, &s) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_empty(&s) == true);

    // 新たにpush
    for (int i = 0; i < 8; ++i) {
        assert(stack_push(&s, &i) == STACK_ERROR_CODE_SUCCESS);
    }

    // popで確認
    for (int i = 7; i >= 0; --i) {
        int out;
        assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
        assert(out == i);
    }

    stack_destroy(&s);

    // 異常系: NULLポインタ
    assert(stack_reserve(5, NULL) == STACK_ERROR_INVALID_ARGUMENT);

    // 異常系: 無効なstack
    stack_t s_invalid = {0};
    assert(stack_reserve(5, &s_invalid) == STACK_ERROR_INVALID_STACK);

    // 異常系: max_element_count_ == 0
    assert(stack_create(sizeof(int), alignof(int), 5, &s) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_reserve(0, &s) == STACK_ERROR_INVALID_ARGUMENT);
    stack_destroy(&s);
}

static void test_stack_empty_and_full(void) {
    stack_t s = STACK_INITIALIZER;
    assert(stack_create(sizeof(int), alignof(int), 3, &s) == STACK_ERROR_CODE_SUCCESS);

    // 初期状態では empty=true, full=false
    assert(stack_empty(&s) == true);
    assert(stack_full(&s) == false);

    // push 1個 → empty=false, full=false
    int value = 42;
    assert(stack_push(&s, &value) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_empty(&s) == false);
    assert(stack_full(&s) == false);

    // push 2個目
    assert(stack_push(&s, &value) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_empty(&s) == false);
    assert(stack_full(&s) == false);

    // push 3個目 → full=true
    assert(stack_push(&s, &value) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_empty(&s) == false);
    assert(stack_full(&s) == true);

    // pop 1個 → full=false
    int out;
    assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_full(&s) == false);

    // pop 残り全部 → empty=true
    assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_pop(&s, &out) == STACK_ERROR_CODE_SUCCESS);
    assert(stack_empty(&s) == true);
    assert(stack_full(&s) == false);

    stack_destroy(&s);

    // 異常系: NULLポインタ
    assert(stack_empty(NULL) == true);  // NULLはemptyとみなす
    assert(stack_full(NULL) == true);  // NULLはfullとみなす
}
