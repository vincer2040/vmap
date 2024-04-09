#ifndef __VBENCH_H__

#define __VBENCH_H__

#include <float.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BENCH_VOLATILE_REG(x) asm volatile("" : "+r"(x) : "r"(x) : "memory")
#define BENCH_VOLATILE_MEM(x) asm volatile("" : "+m"(x) : "m"(x) : "memory")

#define BENCH(title, warmup, samples)                                          \
    for (bench_append(title), bench_internal.i = (warmup) + (samples);         \
         (bench_internal.ns = bench_gettime()), bench_internal.i--;            \
         bench_internal.i < (samples)                                          \
         ? bench_update(bench_gettime() - bench_internal.ns),                  \
                              0 : 0)

typedef struct {
    const char* title;
    size_t count;
    double min;
    double max;
    double mean;
} bench_record;

typedef struct {
    size_t len;
    size_t cap;
    size_t i;
    double ns;
    void (*after_bench)(void*);
    void* after_bench_data;
    bench_record* records;
} bench;

static inline double bench_gettime(void) {
#if defined(CLOCK_PROCESS_CPUTIME_ID)
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return t.tv_nsec * 1.0 / 1000000000 + t.tv_sec;
#elif defined(CLOCK_MONOTONIC_RAW)
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return t.tv_nsec * 1.0 / 1000000000 + t.tv_sec;
#else
    return clock() * 1.0 / CLOCKS_PER_SEC;
#endif
}

static bench bench_internal;

static inline void bench_append(char const* title) {
    bench* b = &bench_internal;
    bench_record* r;
    if (b->len >= b->cap) {
        b->cap = (b->cap << 1) + 1;
        b->records =
            (bench_record*)realloc(b->records, b->cap * sizeof *b->records);
    }
    r = &b->records[b->len++];
    r->mean = r->count = 0;
    r->min = DBL_MAX;
    r->max = DBL_MIN;
    r->title = title;
}

static inline void set_after_bench(void (*after_bench)(void*), void* data) {
    bench* b = &bench_internal;
    b->after_bench = after_bench;
    b->after_bench_data = data;
}

static inline void bench_update(double time) {
    bench* b = &bench_internal;
    bench_record* r = &bench_internal.records[bench_internal.len - 1];
    r->mean += time;
    if (time < r->min) {
        r->min = time;
    }
    if (time > r->max) {
        r->max = time;
    }
    r->count++;
    if (b->after_bench) {
        b->after_bench(b->after_bench_data);
    }
}

static inline int bench_record_cmp(void const* lhs, void const* rhs) {
    bench_record const* l = (bench_record const*)lhs;
    bench_record const* r = (bench_record const*)rhs;
    return l->mean > r->mean ? 1 : -1;
}

static inline void bench_done(void) {
    size_t i, j, maxlen;
    bench* b = &bench_internal;
    qsort(b->records, b->len, sizeof *b->records, bench_record_cmp);

    for (maxlen = i = 0; i < b->len; ++i) {
        size_t l = strlen(b->records[i].title);
        if (l > maxlen) {
            maxlen = l;
        }
    }

    for (i = 0; i < b->len; ++i) {
        int l = printf("%s:", b->records[i].title) - 4;

        for (j = 0; j < maxlen - l; ++j) {
            putchar(' ');
        }

        printf("mean: %.9e,   min: %.9e,   max: %.9e\n",
               b->records[i].mean / b->records[i].count, b->records[i].min,
               b->records[i].max);
    }
    b->len = 0;
}

static inline void bench_free(void) { free(bench_internal.records); }

#endif /* __VBENCH_H__ */
