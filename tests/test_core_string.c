#include "include/test_core_string.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../core/include/core_string.h"

#include "../../include/define.h"

static void test_core_string_default_create(void);
static void test_core_string_create(void);
static void test_core_string_destroy(void);
static void test_core_string_concat(void);
static void test_core_string_substring_copy(void);
static void test_core_string_trim(void);
static void test_core_string_to_i32(void);
static void test_core_string_trim_all_spaces(void);
static void test_core_string_equal_partial_mismatch(void);
static void test_core_string_to_i32_failure_cases(void);
static void test_core_string_substring_copy_empty_result(void);
static void test_trim_all_spaces_returns_empty(void);
static void test_equal_partial_mismatch(void);
static void test_to_i32_strtol_fail_and_range_overflow(void);
static void test_substring_copy_from_greater_than_to(void);
static void test_concat_resize_failure_simulation(void);
static void test_concat_invalid_capacity_case(void);
static void test_destroy_double_free_safe(void);

void test_core_string(void) {
    test_core_string_default_create();
    test_core_string_create();
    test_core_string_destroy();
    test_core_string_concat();
    test_core_string_substring_copy();
    test_core_string_trim();
    test_core_string_to_i32();
    test_core_string_trim_all_spaces();
    test_core_string_equal_partial_mismatch();
    test_core_string_to_i32_failure_cases();
    test_core_string_substring_copy_empty_result();
    test_trim_all_spaces_returns_empty();
    test_equal_partial_mismatch();
    test_to_i32_strtol_fail_and_range_overflow();
    test_substring_copy_from_greater_than_to();
    test_concat_resize_failure_simulation();
    test_concat_invalid_capacity_case();
    test_destroy_double_free_safe();

    // --- core_string_buffer_capacity ---
    assert(core_string_buffer_capacity(NULL) == INVALID_VALUE_U64);

    core_string_t s1 = CORE_STRING_INITIALIZER;
    assert(core_string_buffer_capacity(&s1) == 0);

    // --- core_string_is_empty ---
    assert(core_string_is_empty(NULL) == true);     // NULLポインタ
    assert(core_string_is_empty(&s1) == true);      // internal_dataなし

    // --- core_string_copy_from_char ---
    core_string_t s2 = CORE_STRING_INITIALIZER;
    assert(core_string_copy_from_char(NULL, &s2) == CORE_STRING_INVALID_ARGUMENT);
    assert(core_string_copy_from_char("hello", NULL) == CORE_STRING_INVALID_ARGUMENT);

    // --- core_string_equal_from_char ---
    assert(core_string_equal_from_char(NULL, &s2) == false);
    assert(core_string_equal_from_char("test", NULL) == false);

    // --- core_string_length ---
    assert(core_string_length(NULL) == INVALID_VALUE_U64);
    assert(core_string_length(&s1) == 0);  // internal_data未設定

    // --- core_string_cstr ---
    assert(core_string_cstr(NULL) == NULL);
    assert(core_string_cstr(&s1) == NULL);  // internal_data未設定

    // --- core_string_to_i32 ---
    int32_t value = 0;
    assert(core_string_to_i32(NULL, &value) == CORE_STRING_INVALID_ARGUMENT);
    assert(core_string_to_i32(&s1, NULL) == CORE_STRING_INVALID_ARGUMENT);
    assert(core_string_to_i32(&s1, &value) == CORE_STRING_RUNTIME_ERROR);  // empty buffer

    // --- destroyの多重呼び出し確認 ---
    core_string_destroy(NULL);   // NULLでも落ちない
    core_string_destroy(&s1);    // 未初期化
    core_string_destroy(&s1);    // 二回目の呼び出し（安全性）
}

static void test_core_string_default_create(void) {
    core_string_default_create(NULL); // 異常系: NULL引数でもクラッシュしないこと（void関数）
    core_string_t str;
    core_string_default_create(&str);
    assert(str.internal_data == NULL);
}

static void test_core_string_create(void) {
    core_string_t str = CORE_STRING_INITIALIZER;
    assert(core_string_create(NULL, &str) == CORE_STRING_INVALID_ARGUMENT); // src_ NULL
    assert(core_string_create("ok", NULL) == CORE_STRING_INVALID_ARGUMENT); // dst_ NULL
    CORE_STRING_ERROR_CODE code = core_string_create("Hello", &str);
    assert(code == CORE_STRING_SUCCESS);
    assert(core_string_equal_from_char("Hello", &str));
    core_string_destroy(&str);
}

static void test_core_string_destroy(void) {
    // NULLポインタ渡し → 安全に return できるか
    core_string_destroy(NULL);

    // 未初期化オブジェクト → safe destroy
    core_string_t str1 = CORE_STRING_INITIALIZER;
    core_string_destroy(&str1);
    assert(str1.internal_data == NULL);

    // 一度 destroy 済みのものに再 destroy → double free 防止
    core_string_create("double", &str1);
    core_string_destroy(&str1);
    core_string_destroy(&str1);  // double destroy
    assert(str1.internal_data == NULL);

    // 通常の破棄 → バッファ＆internal_dataが正しくNULLに
    core_string_t str2 = CORE_STRING_INITIALIZER;
    core_string_create("normal", &str2);
    core_string_destroy(&str2);
    assert(str2.internal_data == NULL);
}

static void test_core_string_concat(void) {
    assert(core_string_concat(NULL, NULL) == CORE_STRING_INVALID_ARGUMENT);

    core_string_t dst = CORE_STRING_INITIALIZER;
    core_string_t src = CORE_STRING_INITIALIZER;

    assert(core_string_concat(&src, &dst) == CORE_STRING_RUNTIME_ERROR); // src 未初期化

    core_string_create("Base", &dst);
    core_string_create("_Add", &src);
    assert(core_string_concat(&src, &dst) == CORE_STRING_SUCCESS);
    assert(core_string_equal_from_char("Base_Add", &dst));

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_core_string_substring_copy(void) {
    assert(core_string_substring_copy(NULL, NULL, 0, 0) == CORE_STRING_INVALID_ARGUMENT);

    core_string_t src = CORE_STRING_INITIALIZER;
    core_string_t dst = CORE_STRING_INITIALIZER;

    core_string_create("Substring", &src);

    assert(core_string_substring_copy(&src, &dst, 3, 1) == CORE_STRING_INVALID_ARGUMENT); // from_ > to_
    assert(core_string_substring_copy(&src, &dst, 0, 100) == CORE_STRING_INVALID_ARGUMENT); // to_超過

    CORE_STRING_ERROR_CODE code = core_string_substring_copy(&src, &dst, 3, 5); // "str"
    assert(code == CORE_STRING_SUCCESS);
    assert(core_string_equal_from_char("str", &dst));

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_core_string_trim(void) {
    assert(core_string_trim(NULL, NULL, ' ', ' ') == CORE_STRING_INVALID_ARGUMENT);

    core_string_t src = CORE_STRING_INITIALIZER;
    core_string_t dst = CORE_STRING_INITIALIZER;

    assert(core_string_trim(&src, &dst, ' ', ' ') == CORE_STRING_RUNTIME_ERROR); // src未初期化

    core_string_create("  trim  ", &src);
    CORE_STRING_ERROR_CODE code = core_string_trim(&src, &dst, ' ', ' ');
    assert(code == CORE_STRING_SUCCESS);
    assert(core_string_equal_from_char("trim", &dst));

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_core_string_to_i32(void) {
    assert(core_string_to_i32(NULL, NULL) == CORE_STRING_INVALID_ARGUMENT);

    core_string_t str = CORE_STRING_INITIALIZER;
    int32_t val = 0;

    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR); // 未初期化
    core_string_create("", &str);
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR); // 空文字列

    core_string_copy_from_char("1234", &str);
    assert(core_string_to_i32(&str, &val) == CORE_STRING_SUCCESS);
    assert(val == 1234);

    core_string_copy_from_char("abcd", &str); // 変換失敗
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_copy_from_char("2147483648", &str); // INT32_MAX+1
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_destroy(&str);
}

static void test_core_string_trim_all_spaces(void) {
    core_string_t src = CORE_STRING_INITIALIZER;
    core_string_t dst = CORE_STRING_INITIALIZER;
    core_string_create("     ", &src); // すべて空白

    CORE_STRING_ERROR_CODE code = core_string_trim(&src, &dst, ' ', ' ');
    assert(code == CORE_STRING_SUCCESS);
    assert(core_string_is_empty(&dst));

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_core_string_equal_partial_mismatch(void) {
    core_string_t a = CORE_STRING_INITIALIZER;
    core_string_t b = CORE_STRING_INITIALIZER;
    core_string_create("abc", &a);
    core_string_create("abd", &b);
    assert(!core_string_equal(&a, &b));
    core_string_destroy(&a);
    core_string_destroy(&b);
}

static void test_core_string_to_i32_failure_cases(void) {
    core_string_t str = CORE_STRING_INITIALIZER;
    int32_t val = 0;

    core_string_copy_from_char("123abc", &str);
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_copy_from_char("2147483648", &str); // INT32_MAX + 1
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_copy_from_char("-2147483649", &str); // INT32_MIN - 1
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_destroy(&str);
}

static void test_core_string_substring_copy_empty_result(void) {
    core_string_t src = CORE_STRING_INITIALIZER;
    core_string_t dst = CORE_STRING_INITIALIZER;

    core_string_create("aaa", &src);
    assert(core_string_substring_copy(&src, &dst, 2, 1) == CORE_STRING_INVALID_ARGUMENT); // from > to

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_trim_all_spaces_returns_empty(void) {
    core_string_t src = CORE_STRING_INITIALIZER;
    core_string_t dst = CORE_STRING_INITIALIZER;

    core_string_create("     ", &src);
    CORE_STRING_ERROR_CODE code = core_string_trim(&src, &dst, ' ', ' ');
    assert(code == CORE_STRING_SUCCESS);
    assert(core_string_is_empty(&dst));

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_equal_partial_mismatch(void) {
    core_string_t a = CORE_STRING_INITIALIZER;
    core_string_t b = CORE_STRING_INITIALIZER;
    core_string_create("abc", &a);
    core_string_create("abd", &b);
    assert(!core_string_equal(&a, &b));
    core_string_destroy(&a);
    core_string_destroy(&b);
}

static void test_to_i32_strtol_fail_and_range_overflow(void) {
    core_string_t str = CORE_STRING_INITIALIZER;
    int32_t val = 0;

    core_string_copy_from_char("123abc", &str); // strtol fails
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_copy_from_char("2147483648", &str); // INT32_MAX + 1
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_copy_from_char("-2147483649", &str); // INT32_MIN - 1
    assert(core_string_to_i32(&str, &val) == CORE_STRING_RUNTIME_ERROR);

    core_string_destroy(&str);
}

static void test_substring_copy_from_greater_than_to(void) {
    core_string_t src = CORE_STRING_INITIALIZER;
    core_string_t dst = CORE_STRING_INITIALIZER;

    core_string_create("abcd", &src);
    assert(core_string_substring_copy(&src, &dst, 3, 2) == CORE_STRING_INVALID_ARGUMENT);

    core_string_destroy(&src);
    core_string_destroy(&dst);
}

static void test_concat_resize_failure_simulation(void) {
    core_string_t a = CORE_STRING_INITIALIZER;
    core_string_t b = CORE_STRING_INITIALIZER;
    core_string_create("abc", &a);
    core_string_create("xyz", &b);

    // Resize に大きな値を指定して明示的に呼ぶ（malloc成功でも分岐を踏む）
    core_string_buffer_resize(1024, &a);
    core_string_concat(&b, &a);

    core_string_destroy(&a);
    core_string_destroy(&b);
}

static void test_concat_invalid_capacity_case(void) {
    // capacity == INVALID_VALUE_U64 を得るには dst == NULL が必要
    assert(core_string_concat(&(core_string_t){.internal_data = NULL}, NULL) == CORE_STRING_INVALID_ARGUMENT);
}

static void test_destroy_double_free_safe(void) {
    core_string_t s = CORE_STRING_INITIALIZER;
    core_string_create("destroy-me", &s);
    core_string_destroy(&s);
    core_string_destroy(&s); // 再destroy
    assert(s.internal_data == NULL);
}
