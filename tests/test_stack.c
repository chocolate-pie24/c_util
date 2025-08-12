#include "test_stack.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdalign.h>
#include <inttypes.h>

#include "../include/stack.h"

// ======== テスト用サンプル型 ========

// パディング不要（多分）
typedef struct sample_no_pad_t {
    uint32_t a;
} sample_no_pad_t;

// パディングあり（ゼロ初期化して使う）
typedef struct sample_with_pad_t {
    uint8_t  a;
    uint32_t b;   // a の後に内部パディングが入る可能性
} sample_with_pad_t;

// ポインタを含む
typedef struct sample_with_ptr_t {
    void*    p;
    uint32_t len;
} sample_with_ptr_t;

// ======== ヘルパ ========
static void expect_success(STACK_ERROR_CODE ec) {
    assert(ec == STACK_ERROR_CODE_SUCCESS);
}

static void init_zero(void* p, size_t n) {
    memset(p, 0, n);
}

// ======== 各テスト ========

static void test_create_and_capacity_basic(void) {
    stack_t st = STACK_INITIALIZER;

    // 正常作成
    expect_success(stack_create(sizeof(sample_no_pad_t),
                                alignof(sample_no_pad_t),
                                10, &st));

    // capacity は要素数
    uint64_t cap = 0;
    expect_success(stack_capacity(&st, &cap));
    assert(cap == 10);

    // empty / full
    assert(stack_empty(&st) == true);
    assert(stack_full(&st)  == false);

    stack_destroy(&st);
}

static void test_invalid_alignment_rejected(void) {
    stack_t st = STACK_INITIALIZER;

    // 2の冪乗でない → INVALID_ARGUMENT
    STACK_ERROR_CODE ec = stack_create(sizeof(sample_no_pad_t), /*alignment=*/3, 10, &st);
    assert(ec == STACK_ERROR_INVALID_ARGUMENT);

    // 2の0乗(=1)はOK
    expect_success(stack_create(sizeof(sample_no_pad_t), 1, 10, &st));
    stack_destroy(&st);
}

static void test_push_pop_lifo_no_pad(void) {
    stack_t st = STACK_INITIALIZER;
    expect_success(stack_create(sizeof(sample_no_pad_t),
                                alignof(sample_no_pad_t), 10, &st));

    sample_no_pad_t in = { .a = 1234 };
    expect_success(stack_push(&st, &in));
    assert(stack_empty(&st) == false);

    sample_no_pad_t out = {0};
    expect_success(stack_pop(&st, &out));
    assert(out.a == 1234);
    assert(stack_empty(&st) == true);

    stack_destroy(&st);
}

static void test_push_pop_lifo_with_pad(void) {
    stack_t st = STACK_INITIALIZER;
    expect_success(stack_create(sizeof(sample_with_pad_t),
                                alignof(sample_with_pad_t), 10, &st));

    sample_with_pad_t in;
    init_zero(&in, sizeof(in));   // 内部パディングもゼロにしておく
    in.a = 7; in.b = 0xA5A5A5A5u;

    expect_success(stack_push(&st, &in));

    sample_with_pad_t out;
    init_zero(&out, sizeof(out));
    expect_success(stack_pop(&st, &out));
    assert(out.a == 7);
    assert(out.b == 0xA5A5A5A5u);

    stack_destroy(&st);
}

static void test_push_pop_with_ptr(void) {
    stack_t st = STACK_INITIALIZER;
    expect_success(stack_create(sizeof(sample_with_ptr_t),
                                alignof(sample_with_ptr_t), 10, &st));

    int dummy = 42;
    sample_with_ptr_t in;
    init_zero(&in, sizeof(in));
    in.p = &dummy;
    in.len = 64;

    expect_success(stack_push(&st, &in));

    // peek ポインタ版
    const void* top_ptr = NULL;
    expect_success(stack_pop_peek_ptr(&st, &top_ptr));
    assert(top_ptr != NULL);

    // 中身を比較（コピーを取って検証）
    sample_with_ptr_t peek_copy;
    memcpy(&peek_copy, top_ptr, sizeof(peek_copy));
    assert(peek_copy.p == &dummy);
    assert(peek_copy.len == 64);

    // 破棄して空へ
    expect_success(stack_discard_top(&st));
    assert(stack_empty(&st) == true);

    // pop 空エラー
    sample_with_ptr_t out;
    STACK_ERROR_CODE ec = stack_pop(&st, &out);
    assert(ec == STACK_ERROR_STACK_EMPTY);

    stack_destroy(&st);
}

static void test_full_then_error(void) {
    stack_t st = STACK_INITIALIZER;
    expect_success(stack_create(sizeof(sample_no_pad_t),
                                alignof(sample_no_pad_t), 10, &st));

    sample_no_pad_t in = { .a = 1 };
    for (int i = 0; i < 10; ++i) {
        in.a = (uint32_t)(i+1);
        expect_success(stack_push(&st, &in));
    }
    assert(stack_full(&st) == true);

    // これ以上 push できない
    STACK_ERROR_CODE ec = stack_push(&st, &in);
    assert(ec == STACK_ERROR_STACK_FULL);

    // 全部取り出す（LIFO）
    for (int i = 9; i >= 0; --i) {
        sample_no_pad_t out = {0};
        expect_success(stack_pop(&st, &out));
        assert(out.a == (uint32_t)(i+1));
    }
    assert(stack_empty(&st) == true);

    // さらに pop はエラー
    sample_no_pad_t out = {0};
    ec = stack_pop(&st, &out);
    assert(ec == STACK_ERROR_STACK_EMPTY);

    stack_destroy(&st);
}

static void test_reserve_discards_content(void) {
    stack_t st = STACK_INITIALIZER;
    expect_success(stack_create(sizeof(sample_no_pad_t),
                                alignof(sample_no_pad_t), 10, &st));

    // 2件 push
    sample_no_pad_t in = { .a = 100 };
    expect_success(stack_push(&st, &in));
    in.a = 200;
    expect_success(stack_push(&st, &in));
    assert(stack_empty(&st) == false);

    // reserve(破棄が仕様) → 空になる & capacity 変更
    expect_success(stack_reserve(20, &st));

    uint64_t cap = 0;
    expect_success(stack_capacity(&st, &cap));
    assert(cap == 20);
    assert(stack_empty(&st) == true);

    // 以降 push できる
    in.a = 777;
    expect_success(stack_push(&st, &in));
    sample_no_pad_t out = {0};
    expect_success(stack_pop(&st, &out));
    assert(out.a == 777);

    stack_destroy(&st);
}

static void test_resize_preserves_content(void) {
    stack_t st = STACK_INITIALIZER;
    expect_success(stack_create(sizeof(sample_no_pad_t),
                                alignof(sample_no_pad_t), 4, &st));

    // 3件 push
    for (uint32_t i = 1; i <= 3; ++i) {
        sample_no_pad_t in = { .a = i };
        expect_success(stack_push(&st, &in));
    }

    // resize（拡張のみ可）→ データ保持されているはず
    expect_success(stack_resize(12, &st));
    uint64_t cap = 0;
    expect_success(stack_capacity(&st, &cap));
    assert(cap == 12);

    // LIFO で 3,2,1 の順に出てくる
    for (uint32_t expect = 3; expect >= 1; --expect) {
        sample_no_pad_t out = {0};
        expect_success(stack_pop(&st, &out));
        assert(out.a == expect);
        if (expect == 1) break; // uint32_t なので underflow 防止
    }
    assert(stack_empty(&st) == true);

    stack_destroy(&st);
}

static void test_null_arguments(void) {
    stack_t st = STACK_INITIALIZER;
    STACK_ERROR_CODE ec;

    // NULL stack ptr
    ec = stack_create(sizeof(sample_no_pad_t), alignof(sample_no_pad_t), 10, NULL);
    assert(ec == STACK_ERROR_INVALID_ARGUMENT);

    // 正常作成
    expect_success(stack_create(sizeof(sample_no_pad_t),
                                alignof(sample_no_pad_t), 10, &st));

    // NULL data/out 引数
    ec = stack_push(&st, NULL);
    assert(ec == STACK_ERROR_INVALID_ARGUMENT);

    sample_no_pad_t out = {0};
    ec = stack_pop(NULL, &out);
    assert(ec == STACK_ERROR_INVALID_ARGUMENT);

    ec = stack_capacity(NULL, NULL);
    assert(ec == STACK_ERROR_INVALID_ARGUMENT);

    // stack_full/empty は NULL で true を返す仕様（WARN想定）
    assert(stack_full(NULL)  == true);
    assert(stack_empty(NULL) == true);

    stack_destroy(&st);
}

static void test_error_code_to_string(void) {
    const char* s1 = stack_error_code_to_string(STACK_ERROR_CODE_SUCCESS);
    const char* s2 = stack_error_code_to_string(STACK_ERROR_STACK_EMPTY);
    const char* s3 = stack_error_code_to_string( (STACK_ERROR_CODE)0xFF );
    assert(s1 && s2 && s3);
    // ざっくり non-null だけ確認（厳密な文字列一致は将来の英語統一で変わる可能性があるため避ける）
}


void test_stack(void) {
    puts("=== stack tests start ===");

    test_create_and_capacity_basic();
    test_invalid_alignment_rejected();

    test_push_pop_lifo_no_pad();
    test_push_pop_lifo_with_pad();
    test_push_pop_with_ptr();

    test_full_then_error();

    test_reserve_discards_content();
    test_resize_preserves_content();

    test_null_arguments();
    test_error_code_to_string();

    puts("=== stack tests OK ===");
}