#ifndef __VMAP_H__

#define __VMAP_H__

#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef ht_alloc
#include <stdlib.h>
#define ht_malloc malloc
#define ht_realloc realloc
#define ht_free free
#endif /* ht_alloc */

#ifndef VMAP_INITIAL_CAPACITY
#define VMAP_INITIAL_CAPACITY 32
#else
#if VMAP_INITIAL_CAPACITY == 0
#error please specify an initial capcity larger than 0
#endif
#endif /* VMAP_INITIAL_CAPACITY */

#define VMAP_OK 0
#define VMAP_OOM 1
#define VMAP_NO_KEY 2

#define VMAP_KEY_DELETED (1)
#define VMAP_ENTRY (1 << 1)

#ifdef VMAP_DBG
#include <stdio.h>
#include <stdlib.h>
#define vmap_dbg(...)                                                          \
    do {                                                                       \
        printf("%s:%d ", __FILE__, __LINE__);                                  \
        printf(__VA_ARGS__);                                                   \
        fflush(stdout);                                                        \
    } while (0)
#else
#define vmap_dbg(...)
#endif /* VMAP_DBG */

#define _vmap_init(name, key_type, value_type)                                 \
    typedef int vmap_##name##_key_cmp(const key_type* a, const key_type* b);   \
    typedef void vmap_##name##_free_key_fn(key_type* value);                   \
    typedef void vmap_##name##_free_value_fn(value_type* value);               \
    typedef struct {                                                           \
        int flags;                                                             \
        key_type key;                                                          \
        value_type value;                                                      \
    } vmap_##name##_entry_t;                                                   \
    typedef struct {                                                           \
        uint64_t numel;                                                        \
        uint64_t capacity;                                                     \
        vmap_##name##_key_cmp* key_cmp;                                        \
        vmap_##name##_entry_t entries[];                                       \
    } vmap_##name##_t;

#define _vmap_new(name)                                                        \
    vmap_##name##_t* vmap_##name##_new(vmap_##name##_key_cmp* key_cmp) {       \
        vmap_##name##_t* map;                                                  \
        size_t needed = (sizeof *map) + (sizeof(vmap_##name##_entry_t) *       \
                                         VMAP_INITIAL_CAPACITY);               \
        map = ht_malloc(needed);                                               \
        if (map == NULL) {                                                     \
            return NULL;                                                       \
        }                                                                      \
        memset(map, 0, needed);                                                \
        map->capacity = VMAP_INITIAL_CAPACITY;                                 \
        if (key_cmp) {                                                         \
            map->key_cmp = key_cmp;                                            \
        }                                                                      \
        return map;                                                            \
    }

#define _vmap_hash_init(name, key_type)                                        \
    uint64_t djb2_##name##_hash(const key_type* key) {                         \
        uint64_t hash = 5381;                                                  \
        size_t i, len = sizeof(key_type);                                      \
        const unsigned char* key_ch = ((const unsigned char*)(*key));          \
        for (i = 0; i < len; ++i) {                                            \
            unsigned char ch = key_ch[i];                                      \
            hash = ((hash << 5) + hash) + ch;                                  \
        }                                                                      \
        return hash;                                                           \
    }

#define _vmap_entry_new(name, key_type, value_type)                            \
    vmap_##name##_entry_t vmap_##name##_entry_new(key_type* key,               \
                                                  value_type* value) {         \
        vmap_##name##_entry_t entry = {0};                                     \
        entry.flags |= VMAP_ENTRY;                                             \
        memcpy(&entry.key, key, sizeof(key_type));                             \
        memcpy(&entry.value, value, sizeof(value_type));                       \
        return entry;                                                          \
    }

#define _vmap_insert(name, key_type, value_type)                               \
    int vmap_##name##_insert(vmap_##name##_t** map, key_type* key,             \
                             value_type* value) {                              \
        uint64_t i, hash, numel = (*map)->numel, capacity = (*map)->capacity;  \
        vmap_##name##_entry_t* e;                                              \
        if (numel == capacity) {                                               \
            return VMAP_OOM;                                                   \
        }                                                                      \
        hash = djb2_##name##_hash(key) % capacity;                             \
        vmap_dbg("insert hash %lu\n", hash);                                   \
        for (;;) {                                                             \
            e = &((*map)->entries[hash]);                                      \
            if (e->flags & VMAP_ENTRY) {                                       \
                if ((*map)->key_cmp) {                                         \
                    if ((*map)->key_cmp(&e->key, key) == 0) {                  \
                        memcpy(&e->value, value, sizeof(value_type));          \
                        return VMAP_OK;                                        \
                    }                                                          \
                    hash = (hash + 1) % capacity;                              \
                } else {                                                       \
                    if (memcmp(&e->key, key, sizeof(key_type)) == 0) {         \
                        memcpy(&e->value, value, sizeof(value_type));          \
                        return VMAP_OK;                                        \
                    }                                                          \
                    hash = (hash + 1) % capacity;                              \
                }                                                              \
            } else if (e->flags & VMAP_KEY_DELETED) {                          \
                vmap_##name##_entry_t new_entry =                              \
                    vmap_##name##_entry_new(key, value);                       \
                (*map)->entries[hash] = new_entry;                             \
                (*map)->numel++;                                               \
                return VMAP_OK;                                                \
            } else {                                                           \
                vmap_##name##_entry_t new_entry =                              \
                    vmap_##name##_entry_new(key, value);                       \
                (*map)->entries[hash] = new_entry;                             \
                (*map)->numel++;                                               \
                return VMAP_OK;                                                \
            }                                                                  \
        }                                                                      \
        return VMAP_OK;                                                        \
    }

#define _vmap_find(name, key_type, value_type)                                 \
    const value_type* vmap_##name##_find(vmap_##name##_t* map,                 \
                                         const key_type* key) {                \
        uint64_t hash, numel = map->numel, capacity = map->capacity;           \
        vmap_##name##_entry_t* e;                                              \
        if (numel == 0) {                                                      \
            return NULL;                                                       \
        }                                                                      \
        hash = djb2_##name##_hash(key) % capacity;                             \
        vmap_dbg("find hash %lu\n", hash);                                     \
        for (;;) {                                                             \
            e = &(map->entries[hash]);                                         \
            if (e->flags == 0) {                                               \
                break;                                                         \
            }                                                                  \
            if (e->flags & VMAP_ENTRY) {                                       \
                if (map->key_cmp) {                                            \
                    if (map->key_cmp((const key_type*)(&e->key), key) == 0) {  \
                        return &e->value;                                      \
                    } else {                                                   \
                        hash = (hash + 1) % capacity;                          \
                        continue;                                              \
                    }                                                          \
                } else {                                                       \
                    if (memcmp(&e->key, key, sizeof(key_type)) == 0) {         \
                        return &e->value;                                      \
                    } else {                                                   \
                        hash = (hash + 1) % capacity;                          \
                        continue;                                              \
                    }                                                          \
                }                                                              \
            } else if (e->flags & VMAP_KEY_DELETED) {                          \
                hash = (hash + 1) % capacity;                                  \
                continue;                                                      \
            }                                                                  \
            break;                                                             \
        }                                                                      \
        return NULL;                                                           \
    }

#define _vmap_delete(name, key_type, value_type)                               \
    int vmap_##name##_delete(vmap_##name##_t* map, const key_type* key,        \
                             vmap_##name##_free_key_fn* free_key,              \
                             vmap_##name##_free_value_fn* free_value) {        \
        uint64_t i, hash, numel = map->numel, capacity = map->capacity;        \
        vmap_##name##_entry_t* e;                                              \
        if (numel == 0) {                                                      \
            return VMAP_NO_KEY;                                                \
        }                                                                      \
        hash = djb2_##name##_hash(key) % capacity;                             \
        vmap_dbg("delete hash %lu\n", hash);                                   \
        for (;;) {                                                             \
            e = &(map->entries[hash]);                                         \
            if (e->flags == 0) {                                               \
                break;                                                         \
            }                                                                  \
            if (e->flags & VMAP_ENTRY) {                                       \
                if (map->key_cmp) {                                            \
                    if (map->key_cmp((const key_type*)(&e->key), key) == 0) {  \
                        if (free_key) {                                        \
                            free_key(&e->key);                                 \
                        }                                                      \
                        if (free_value) {                                      \
                            free_value(&e->value);                             \
                        }                                                      \
                        memset(&e->key, 0, sizeof(key_type));                  \
                        memset(&e->value, 0, sizeof(value_type));              \
                        e->flags = 0;                                          \
                        e->flags |= VMAP_KEY_DELETED;                          \
                        return VMAP_OK;                                        \
                    } else {                                                   \
                        hash = (hash + 1) % capacity;                          \
                        continue;                                              \
                    }                                                          \
                } else {                                                       \
                    if (memcmp(&e->key, key, sizeof(key_type)) == 0) {         \
                        if (free_key) {                                        \
                            free_key(&e->key);                                 \
                        }                                                      \
                        if (free_value) {                                      \
                            free_value(&e->value);                             \
                        }                                                      \
                        memset(&e->key, 0, sizeof(key_type));                  \
                        memset(&e->value, 0, sizeof(value_type));              \
                        e->flags = 0;                                          \
                        e->flags |= VMAP_KEY_DELETED;                          \
                        return VMAP_OK;                                        \
                    } else {                                                   \
                        hash = (hash + 1) % capacity;                          \
                        continue;                                              \
                    }                                                          \
                }                                                              \
            } else if (e->flags & VMAP_KEY_DELETED) {                          \
                hash = (hash + 1) % capacity;                                  \
                continue;                                                      \
            }                                                                  \
            break;                                                             \
        }                                                                      \
        return VMAP_NO_KEY;                                                    \
    }

#define _vmap_free(name)                                                       \
    void vmap_##name##_free(vmap_##name##_t* map,                              \
                            vmap_##name##_free_key_fn* free_key,               \
                            vmap_##name##_free_value_fn* free_value) {         \
        uint64_t i, len = map->capacity;                                       \
        if (free_key && free_value) {                                          \
            for (i = 0; i < len; ++i) {                                        \
                vmap_##name##_entry_t* e = &map->entries[i];                   \
                if (e->flags & VMAP_ENTRY) {                                   \
                    free_key(&e->key);                                         \
                    free_value(&e->value);                                     \
                }                                                              \
            }                                                                  \
        } else if (free_key) {                                                 \
            for (i = 0; i < len; ++i) {                                        \
                vmap_##name##_entry_t* e = &map->entries[i];                   \
                if (e->flags & VMAP_ENTRY) {                                   \
                    free_key(&e->key);                                         \
                }                                                              \
            }                                                                  \
        } else if (free_value) {                                               \
            for (i = 0; i < len; ++i) {                                        \
                vmap_##name##_entry_t* e = &map->entries[i];                   \
                if (e->flags & VMAP_ENTRY) {                                   \
                    free_value(&e->value);                                     \
                }                                                              \
            }                                                                  \
        }                                                                      \
        ht_free(map);                                                          \
    }

#define VMAP_INIT(name, key_type, value_type)                                  \
    _vmap_init(name, key_type, value_type) _vmap_new(name)                     \
        _vmap_hash_init(name, key_type)                                        \
            _vmap_entry_new(name, key_type, value_type)                        \
                _vmap_insert(name, key_type, value_type)                       \
                    _vmap_find(name, key_type, value_type)                     \
                        _vmap_delete(name, key_type, value_type)               \
                            _vmap_free(name)

#define vmap(name) vmap_##name##_t*
#define vmap_new(name) vmap_##name##_new
#define vmap_insert(name) vmap_##name##_insert
#define vmap_find(name) vmap_##name##_find
#define vmap_delete(name) vmap_##name##_delete
#define vmap_free(name) vmap_##name##_free

#endif /* __VMAP_H__ */
