/**
 *
 * Author: Vince Ronnebaum <ronnebaumvince AT gmail DOT com>
 * Basic unit testing library for C projects
 *
 * MIT License
 *
 * Copyright (c) 2024 vincer2040
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __VTEST_H__

#define __VTEST_H__

#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST(name) void test_##name(void)

static int tests_failed = 0;

#define vassert(cond)                                                          \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion %s failed\n", #cond);                   \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define vassert_int_eq(a, b)                                                   \
    do {                                                                       \
        if ((a) != (b)) {                                                      \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s != %s, ", #a, #b);           \
            fprintf(stderr, "%s = %d, %s = %d\n", #a, (a), #b, (b));           \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define vassert_uint_eq(a, b)                                                  \
    do {                                                                       \
        if ((a) != (b)) {                                                      \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s != %s, ", #a, #b);           \
            fprintf(stderr, "%s = %u, %s = %u\n", #a, (a), #b, (b));           \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define vassert_ptr_nonnull(a)                                                 \
    do {                                                                       \
        if ((a) == NULL) {                                                     \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s != NULL, ", #a);             \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define vassert_ptr_null(a)                                                    \
    do {                                                                       \
        if ((a) != NULL) {                                                     \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s == NULL, ", #a);             \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define vassert_mem_eq(a, b, size)                                             \
    do {                                                                       \
        if (memcmp((a), (b), (size)) != 0) {                                   \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s != %s\n", #a, #b);           \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define vassert_str_eq(a, b)                                                   \
    do {                                                                       \
        size_t a_len = strlen((a));                                            \
        size_t b_len = strlen((b));                                            \
        if (a_len != b_len) {                                                  \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s != %s, ", #a, #b);           \
            fprintf(stderr, "%s = %s, %s = %s\n", #a, (a), #b, (b));           \
            tests_failed = 1;                                                  \
        }                                                                      \
        if (strcmp((a), (b)) != 0) {                                           \
            fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                     \
            fprintf(stderr, "assertion failed: %s != %s, ", #a, #b);           \
            fprintf(stderr, "%s = %s, %s = %s\n", #a, (a), #b, (b));           \
            tests_failed = 1;                                                  \
        }                                                                      \
    } while (0)

#define run_test(name)                                                         \
    do {                                                                       \
        test_##name();                                                         \
    } while (0)

#define tests_done()                                                           \
    do {                                                                       \
        if (tests_failed) {                                                    \
            fprintf(stderr, "tests failed\n");                                 \
            exit(1);                                                           \
        }                                                                      \
        printf("all tests passed\n");                                          \
        exit(0);                                                               \
    } while (0)

#endif /* __VTEST_H__ */
