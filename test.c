#include "vmap.h"
#include "vtest.h"
#include <assert.h>
#include <stdlib.h>

#define KEY_SIZE 100
#define arr_size(arr) sizeof arr / sizeof arr[0]
typedef char key[KEY_SIZE];

typedef struct {
    key k;
    int value;
} key_val;

uint64_t hash(const void* k) {
    uint64_t hash = 5381;
    char ch;
    size_t i;
    char* my_key = (char*)k;
    for (i = 0; i < KEY_SIZE; ++i) {
        ch = my_key[i];
        hash = ((hash << 5) + hash) + ch;
    }
    return hash;
}

vmap_type* init_type(void) {
    vmap_type* t = malloc(sizeof *t);
    assert(t != NULL);
    memset(t, 0, sizeof *t);
    t->hash = hash;
    t->key_size = sizeof(key);
    t->value_size = sizeof(int);
    return t;
}

TEST(it_works) {
    vmap* map = vmap_new(init_type());
    key_val kvs[] = {
        {"foo", 0},       {"foobar", 1},     {"foobaz", 2},
        {"foobarbaz", 3}, {"foobazbar", 4},  {"bar", 5},
        {"barfoo", 6},    {"barbaz", 7},     {"barfoobaz", 8},
        {"barbazfoo", 9}, {"baz", 9},        {"bazfoo", 10},
        {"bazbar", 11},   {"bazfoobar", 12}, {"bazbarfoo", 13},
        {"vmap", 14},     {"vmap_t", 15},    {"vmap_entry", 16},
        {"abc", 17},      {"xyz", 18},       {"cba", 19},
        {"zyx", 20},      {"abcd", 21},      {"abcde", 22},
    };
    size_t i, len = arr_size(kvs);
    for (i = 0; i < len; ++i) {
        key_val kv = kvs[i];
        int res = vmap_insert(&map, kv.k, &kv.value);
        vassert_int_eq(res, VMAP_OK);
    }
    for (i = 0; i < len; ++i) {
        key_val kv = kvs[i];
        const int* res = vmap_find(map, kv.k);
        vassert(res != NULL);
        vassert_int_eq(*res, kv.value);
    }
    for (i = 0; i < len; ++i) {
        key_val kv = kvs[i];
        int res = vmap_erase(&map, kv.k);
        vassert_int_eq(res, VMAP_OK);
    }
    for (i = 0; i < len; ++i) {
        key_val kv = kvs[i];
        int res = vmap_insert(&map, kv.k, &kv.value);
        vassert_int_eq(res, VMAP_OK);
    }
    for (i = 0; i < len; ++i) {
        key_val kv = kvs[i];
        const int* res = vmap_find(map, kv.k);
        vassert(res != NULL);
        vassert_int_eq(*res, kv.value);
    }
    vmap_delete(map);
}

int main(void) {
    run_test(it_works);
    tests_done();
    return 0;
}
