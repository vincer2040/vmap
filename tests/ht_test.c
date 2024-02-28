#define VMAP_DBG
#include "../src/vmap.h"
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char key_type[100];

VMAP_INIT(my, key_type, int)

START_TEST(test_it_works) {
    vmap(my) map = vmap_new(my)(NULL);
    key_type k1 = "foo";
    key_type k2 = "bar";
    key_type k3 = "baz";
    key_type k4 = "foobar";
    key_type k5 = "barbaz";
    key_type k6 = "foobarbaz";
    key_type not_in = "barbazfoo";
    int v1 = 1;
    int v2 = 2;
    int v3 = 3;
    int v4 = 4;
    int v5 = 5;
    int v6 = 6;
    ck_assert_int_eq(vmap_insert(my)(&map, &k1, &v1), VMAP_OK);
    ck_assert_int_eq(vmap_insert(my)(&map, &k2, &v2), VMAP_OK);
    ck_assert_int_eq(vmap_insert(my)(&map, &k3, &v3), VMAP_OK);
    ck_assert_int_eq(vmap_insert(my)(&map, &k4, &v4), VMAP_OK);
    ck_assert_int_eq(vmap_insert(my)(&map, &k5, &v5), VMAP_OK);
    ck_assert_int_eq(vmap_insert(my)(&map, &k6, &v6), VMAP_OK);

    const int* f1 = vmap_find(my)(map, &k1);
    ck_assert_ptr_nonnull(f1);
    ck_assert_int_eq(*f1, v1);

    const int* f2 = vmap_find(my)(map, &k2);
    ck_assert_ptr_nonnull(f2);
    ck_assert_int_eq(*f2, v2);

    const int* f3 = vmap_find(my)(map, &k3);
    ck_assert_ptr_nonnull(f3);
    ck_assert_int_eq(*f3, v3);

    const int* f4 = vmap_find(my)(map, &k4);
    ck_assert_ptr_nonnull(f4);
    ck_assert_int_eq(*f4, v4);

    const int* f5 = vmap_find(my)(map, &k5);
    ck_assert_ptr_nonnull(f5);
    ck_assert_int_eq(*f5, v5);

    const int* f6 = vmap_find(my)(map, &k6);
    ck_assert_ptr_nonnull(f6);
    ck_assert_int_eq(*f6, v6);

    const int* not_found = vmap_find(my)(map, &not_in);
    ck_assert_ptr_null(not_found);

    ck_assert_int_eq(vmap_delete(my)(map, &k1, NULL, NULL), VMAP_OK);
    ck_assert_int_eq(vmap_delete(my)(map, &k2, NULL, NULL), VMAP_OK);
    ck_assert_int_eq(vmap_delete(my)(map, &k3, NULL, NULL), VMAP_OK);
    ck_assert_int_eq(vmap_delete(my)(map, &k4, NULL, NULL), VMAP_OK);
    ck_assert_int_eq(vmap_delete(my)(map, &k5, NULL, NULL), VMAP_OK);
    ck_assert_int_eq(vmap_delete(my)(map, &k6, NULL, NULL), VMAP_OK);

    const int* deleted1 = vmap_find(my)(map, &k1);
    ck_assert_ptr_null(deleted1);

    const int* deleted2 = vmap_find(my)(map, &k2);
    ck_assert_ptr_null(deleted2);

    const int* deleted3 = vmap_find(my)(map, &k3);
    ck_assert_ptr_null(deleted3);

    const int* deleted4 = vmap_find(my)(map, &k4);
    ck_assert_ptr_null(deleted4);

    const int* deleted5 = vmap_find(my)(map, &k5);
    ck_assert_ptr_null(deleted5);

    const int* deleted6 = vmap_find(my)(map, &k6);
    ck_assert_ptr_null(deleted6);

    vmap_free(my)(map, NULL, NULL);
}
END_TEST

Suite* suite() {
    Suite* s;
    TCase* tc_core;
    s = suite_create("ht");
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_it_works);
    suite_add_tcase(s, tc_core);
    return s;
}

int main() {
    int number_failed;
    Suite* s;
    SRunner* sr;
    s = suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
