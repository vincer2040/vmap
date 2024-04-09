#ifndef __VMAP_H__

#define __VMAP_H__

#include "vmap_config.h"
#include <memory.h>
#include <stddef.h>
#include <stdint.h>

#ifndef VMAP_ALLOC
#include <stdlib.h>
#define vmap_malloc malloc
#define vmap_calloc calloc
#define vmap_realloc realloc
#define vmap_free free
#endif /* VMAP_ALLOC */

#define VMAP_OK 0
#define VMAP_OOM 1
#define VMAP_NO_KEY 2

typedef struct vmap vmap;

typedef struct {
    uint64_t (*hash)(const void* key);
    int (*key_cmp)(const void* a, const void* b);
    void (*key_free)(void* key);
    void (*value_free)(void* value);
    size_t key_size;
    size_t value_size;
} vmap_type;

vmap* vmap_new(vmap_type* type);
void vmap_delete(vmap* map);
int vmap_insert(vmap** map, void* key, void* value);
const void* vmap_find(vmap* map, const void* key);
int vmap_erase(vmap** map, const void* key);

#endif /* __VMAP_H__ */
