
#include <assert.h>
#include <string.h>

#include "include/test_dynamic_array.h"

#include "containers/dynamic_array.h"

typedef struct {
    int id;
    float value;
} test_object_t;

typedef struct __attribute__((packed)) {
    uint8_t x;
    uint16_t y;
    uint32_t z;
} unaligned7_t;

static void test_create_and_destroy(void);
static void test_push_and_ref(void);
static void test_resize_expand_only(void);
static void test_element_out_of_range(void);
static void test_alignment_and_padding_behavior(void);
static void test_null_pointer_handling();
static void test_uninitialized_dynamic_array(void);
static void test_push_overflow(void);

void test_dynamic_array(void) {
    test_create_and_destroy();
    test_push_and_ref();
    test_resize_expand_only();
    test_element_out_of_range();
    test_alignment_and_padding_behavior();

    test_null_pointer_handling();
    test_uninitialized_dynamic_array();
    test_push_overflow();
}

static void test_create_and_destroy(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    DYNAMIC_ARRAY_ERROR_CODE result;

    result = dynamic_array_create(sizeof(test_object_t), alignof(test_object_t), 10, &array);
    assert(result == DYNAMIC_ARRAY_SUCCESS);

    dynamic_array_destroy(&array);
}

static void test_push_and_ref(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    dynamic_array_create(sizeof(test_object_t), alignof(test_object_t), 3, &array);

    test_object_t obj = { 42, 3.14f };
    dynamic_array_element_push(&obj, &array);

    test_object_t out = { 0 };
    dynamic_array_element_ref(0, &array, &out);

    assert(out.id == 42);
    assert(out.value == 3.14f);

    dynamic_array_destroy(&array);
}

static void test_resize_expand_only(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    dynamic_array_create(sizeof(test_object_t), alignof(test_object_t), 2, &array);

    test_object_t obj = { 1, 1.0f };
    dynamic_array_element_push(&obj, &array);
    dynamic_array_element_push(&obj, &array);

    DYNAMIC_ARRAY_ERROR_CODE res = dynamic_array_resize(1, &array); // should fail
    assert(res == DYNAMIC_ARRAY_INVALID_ARGUMENT);

    res = dynamic_array_resize(5, &array); // should succeed
    assert(res == DYNAMIC_ARRAY_SUCCESS);

    dynamic_array_destroy(&array);
}

static void test_element_out_of_range(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    dynamic_array_create(sizeof(test_object_t), alignof(test_object_t), 2, &array);

    test_object_t out = { 0 };
    DYNAMIC_ARRAY_ERROR_CODE res = dynamic_array_element_ref(5, &array, &out);
    assert(res == DYNAMIC_ARRAY_OUT_OF_RANGE);

    dynamic_array_destroy(&array);
}

static void test_alignment_and_padding_behavior(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    DYNAMIC_ARRAY_ERROR_CODE result;

    result = dynamic_array_create(sizeof(unaligned7_t), alignof(unaligned7_t), 5, &array);
    assert(result == DYNAMIC_ARRAY_SUCCESS);

    unaligned7_t obj = { 1, 0x0203, 0x04050607 };
    result = dynamic_array_element_push(&obj, &array);
    assert(result == DYNAMIC_ARRAY_SUCCESS);

    unaligned7_t out = { 0 };
    result = dynamic_array_element_ref(0, &array, &out);
    assert(result == DYNAMIC_ARRAY_SUCCESS);

    assert(out.x == 1);
    assert(out.y == 0x0203);
    assert(out.z == 0x04050607);

    uint64_t capacity = 0;
    result = dynamic_array_capacity(&array, &capacity);
    assert(result == DYNAMIC_ARRAY_SUCCESS);
    assert(capacity == 5);

    dynamic_array_destroy(&array);
}

typedef struct {
    int id;
    float value;
} test_object2_t;

static void test_null_pointer_handling(void) {
    DYNAMIC_ARRAY_ERROR_CODE result;

    // NULL渡し: create
    result = dynamic_array_create(sizeof(test_object2_t), alignof(test_object2_t), 10, NULL);
    assert(result == DYNAMIC_ARRAY_INVALID_ARGUMENT);

    // NULL渡し: destroy
    dynamic_array_destroy(NULL); // should not crash

    // NULL渡し: push/ref/set
    result = dynamic_array_element_push(NULL, NULL);
    assert(result == DYNAMIC_ARRAY_INVALID_ARGUMENT);

    result = dynamic_array_element_ref(0, NULL, NULL);
    assert(result == DYNAMIC_ARRAY_INVALID_ARGUMENT);

    result = dynamic_array_element_set(0, NULL, NULL);
    assert(result == DYNAMIC_ARRAY_INVALID_ARGUMENT);
}

static void test_uninitialized_dynamic_array(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    test_object2_t dummy;

    DYNAMIC_ARRAY_ERROR_CODE res;

    // ref from uninitialized array
    res = dynamic_array_element_ref(0, &array, &dummy);
    assert(res == DYNAMIC_ARRAY_INVALID_DARRAY);

    // size/capacity from uninitialized array
    uint64_t size;
    res = dynamic_array_size(&array, &size);
    assert(res == DYNAMIC_ARRAY_INVALID_DARRAY);

    uint64_t cap;
    res = dynamic_array_capacity(&array, &cap);
    assert(res == DYNAMIC_ARRAY_INVALID_DARRAY);
}

static void test_push_overflow(void) {
    dynamic_array_t array = DYNAMIC_ARRAY_INITIALIZER;
    dynamic_array_create(sizeof(test_object2_t), alignof(test_object2_t), 1, &array);

    test_object2_t obj = { 10, 1.23f };
    dynamic_array_element_push(&obj, &array);

    // 2個目のpushは失敗する
    DYNAMIC_ARRAY_ERROR_CODE res = dynamic_array_element_push(&obj, &array);
    assert(res == DYNAMIC_ARRAY_BUFFER_FULL);

    dynamic_array_destroy(&array);
}
