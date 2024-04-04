#include "vbench.h"
#include "vmap.h"
#include "util/util.h"
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

void run_bench_32(void) {
    size_t i, len = num_keys;
    for (i = 0; i < len; ++i) {
        key_val kv = key_vals[i];
        printf("here: %s %d\n", kv.key, kv.value);
    }
}

int main(void) {
    size_t len = 0;
    char* file_contents;

    printf("BENCH MARKING %d byte SIZE KEYS\n", KEY_SIZE);

    file_contents = read_file(NULL, &len);
    assert(file_contents != NULL);
    init_key_vals(file_contents, len);
    free(file_contents);

    run_bench_32();

    free(key_vals);
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
