#include "util/util.h"
#include "vbench.h"
#include "vmap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef KEY_SIZE
#define KEY_SIZE 4
#endif /* KEY_SIZE */

typedef struct {
    char key[KEY_SIZE + 1];
    int value;
} key_val;

void init_key_vals(const char* input, size_t input_len);

size_t num_keys = 0;
key_val* key_vals;

uint64_t hash(const void* k) {
    uint64_t hash = 5381;
    char ch;
    size_t i;
    const char* my_key = k;
    for (i = 0; i < KEY_SIZE; ++i) {
        ch = my_key[i];
        hash = ((hash << 5) + hash) + ch;
    }
    return hash;
}

vmap_type* init_type(void) {
    vmap_type* t = calloc(1, sizeof *t);
    assert(t != NULL);
    t->hash = hash;
    t->key_size = KEY_SIZE;
    t->value_size = sizeof(int);
    return t;
}

void run_bench(const char* bench_name, size_t len, size_t warmup,
               size_t samples) {
    size_t i;
    vmap* map;
    char* key;
    int value;
    assert(len < num_keys);
    map = vmap_new(init_type());
    for (i = 0; i < len; ++i) {
        char* key = key_vals[i].key;
        int value = key_vals[i].value;
        int res = vmap_insert(&map, key, &value);
        assert(res == 0);
    }
    i++;
    assert(i < num_keys);
    key = key_vals[i].key;
    value = key_vals[i].value;
    BENCH(bench_name, warmup, samples) {
        int res = vmap_insert(&map, key, &value);
        BENCH_VOLATILE_REG(res);
    }
    vmap_delete(map);
}

int main(void) {
    size_t len = 0;
    char* file_contents;

    printf("BENCH MARKING %d byte SIZE KEYS\n", KEY_SIZE);

    file_contents = read_file(NULL, &len);
    assert(file_contents != NULL);
    init_key_vals(file_contents, len);
    free(file_contents);

    free(key_vals);

    bench_free();
    return 0;
}

void init_key_vals(const char* input, size_t input_len) {
    line_iter iter = line_iter_new(input, input_len);
    size_t kvs_len = 0, kvs_cap = 32;
    key_vals = calloc(kvs_cap, sizeof *key_vals);
    if (key_vals == NULL) {
        fprintf(stderr, "failed to allocate memory for key vals\n");
        exit(1);
    }
    while (iter.cur) {
        const char* cur = iter.cur;
        size_t cur_len = iter.cur_len;
        size_t i = 0;
        char ch;
        key_val kv = {0};
        if (cur_len == 0) {
            break;
        }
        while ((ch = cur[i]) != ' ' && i < KEY_SIZE) {
            kv.key[i] = ch;
            i++;
        }
        i++;
        while ((ch = cur[i]) != '\n') {
            kv.value = (kv.value * 10) + (ch - '0');
            i++;
        }
        if (kvs_len == kvs_cap) {
            void* tmp;
            kvs_cap <<= 1;
            tmp = realloc(key_vals, kvs_cap * (sizeof *key_vals));
            if (tmp == NULL) {
                free(key_vals);
                fprintf(stderr, "failed to allocate memory for key vals\n");
                exit(1);
            }
            key_vals = tmp;
        }
        key_vals[kvs_len] = kv;
        kvs_len++;
        line_iter_next(&iter);
    }
    num_keys = kvs_len;
}
