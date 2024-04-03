#include "vmap.h"

#define VMAP_INITIAL_POWER 5
#define VMAP_EMPTY (0)
#define VMAP_DELETED (1 << 0)
#define VMAP_FULL (1 << 1)

#define VMAP_MAX_LOAD .7
#define VMAP_MIN_LOAD .3

#define vmap_key_free(map, key)                                                \
    do {                                                                       \
        if ((map)->type->key_free) {                                           \
            (map)->type->key_free((key));                                      \
        }                                                                      \
    } while (0)

#define vmap_value_free(map, value)                                            \
    do {                                                                       \
        if ((map)->type->value_free) {                                         \
            (map)->type->value_free((value));                                  \
        }                                                                      \
    } while (0)

#define vmap_key_cmp(map, a, b)                                                \
    (map)->type->key_cmp ? (map)->type->key_cmp((a), (b))                      \
                         : memcmp((a), (b), (map)->type->key_size)

struct vmap_entry {
    uint8_t flags;
    unsigned char data[];
};

struct vmap {
    uint64_t numel;
    uint64_t numelplusdeleted;
    uint64_t power;
    size_t padding;
    vmap_type* type;
    vmap_entry* entries[];
};

#define VMAP_HDR_SIZE (uintptr_t)(&((vmap_entry*)NULL)->data)

#define vmap_padding(key_size)                                                 \
    ((sizeof(void*) - ((((key_size)) + VMAP_HDR_SIZE) % sizeof(void*))) &        \
     (sizeof(void*) - 1))

static int vmap_resize(vmap** map, uint64_t new_power);
static vmap* vmap_new_with_cap(vmap_type* type, uint64_t power, size_t padding);
static vmap_entry* vmap_entry_new(vmap* map, void* key, void* value);

vmap* vmap_new(vmap_type* type) {
    vmap* map;
    size_t initial_cap = (1 << VMAP_INITIAL_POWER);
    size_t needed = (sizeof *map) + (sizeof(vmap_entry*) * initial_cap);
    size_t padding;
    if (type->hash == NULL) {
        return NULL;
    }
    if (type->key_size == 0) {
        return NULL;
    }
    if (type->value_size == 0) {
        return NULL;
    }
    padding = vmap_padding(type->key_size);
    map = vmap_malloc(needed);
    memset(map, 0, needed);
    map->type = type;
    map->power = VMAP_INITIAL_POWER;
    map->padding = padding;
    return map;
}

int vmap_insert(vmap** map, void* key, void* value) {
    vmap* m = *map;
    uint64_t cap = (1 << m->power);
    uint64_t hash = m->type->hash(key) & (cap - 1);
    size_t value_size = m->type->value_size;
    size_t key_size = m->type->key_size;
    size_t offset = key_size + m->padding;
    vmap_entry* new_entry;
    while (1) {
        vmap_entry* e = m->entries[hash];
        double new_load;
        if (e == NULL) {
            new_entry = vmap_entry_new(m, key, value);
            if (new_entry == NULL) {
                return VMAP_OOM;
            }
            m->entries[hash] = new_entry;
            m->numel++;
            m->numelplusdeleted++;
            new_load = (double)m->numelplusdeleted / (double)cap;
            if (new_load > VMAP_MAX_LOAD) {
                return vmap_resize(map, m->power + 1);
            }
            break;
        }
        if (e->flags & VMAP_DELETED) {
            hash = (hash + 1) & (cap - 1);
            continue;
        }
        if (e->flags & VMAP_FULL) {
            int cmp = vmap_key_cmp(m, key, value);
            if (cmp != 0) {
                hash = (hash + 1) & (cap - 1);
                continue;
            }
            vmap_key_free(m, key);
            vmap_value_free(m, e->data + offset);
            memcpy(e->data + offset, value, value_size);
            break;
        }
        // e->flags = VMAP_EMPTY
        memcpy(e->data, key, key_size);
        memcpy(e->data + offset, value, value_size);
        e->flags = VMAP_FULL;
        m->numel++;
        m->numelplusdeleted++;
        new_load = (double)m->numelplusdeleted / (double)cap;
        if (new_load > VMAP_MAX_LOAD) {
            return vmap_resize(map, m->power + 1);
        }
        break;
    }
    return VMAP_OK;
}

const void* vmap_find(vmap* map, const void* key) {
    uint64_t cap = (1 << map->power);
    uint64_t hash = map->type->hash(key) & (cap - 1);
    size_t key_size = map->type->key_size;
    size_t offset = key_size + map->padding;
    while (1) {
        vmap_entry* e = map->entries[hash];
        int cmp;
        if (e == NULL) {
            break;
        }
        if (e->flags & VMAP_EMPTY) {
            break;
        }
        if (e->flags & VMAP_DELETED) {
            hash = (hash + 1) & (cap - 1);
            continue;
        }
        // e->flags = VMAP_FULL
        cmp = vmap_key_cmp(map, e->data, key);
        if (cmp != 0) {
            hash = (hash + 1) & (cap - 1);
            continue;
        }
        return e->data + offset;
    }
    return NULL;
}

int vmap_erase(vmap** map, const void* key) {
    vmap* m = *map;
    uint64_t cap = (1 << m->power);
    uint64_t hash = m->type->hash(key) & (cap - 1);
    size_t key_size = m->type->key_size;
    size_t offset = key_size + m->padding;

    while (1) {
        vmap_entry* e = m->entries[hash];
        int cmp;
        double new_load;
        if (e == NULL) {
            break;
        }
        if (e->flags & VMAP_EMPTY) {
            break;
        }
        if (e->flags & VMAP_DELETED) {
            hash = (hash + 1) & (cap - 1);
            continue;
        }
        // e->flags = VMAP_FULL
        cmp = vmap_key_cmp(m, e->data, key);
        if (cmp != 0) {
            hash = (hash + 1) & (cap - 1);
            continue;
        }
        vmap_value_free(m, e->data + offset);
        vmap_key_free(m, e->data);
        e->flags = VMAP_DELETED;
        m->numel--;
        new_load = (double)m->numel / (double)cap;
        if ((new_load < VMAP_MIN_LOAD) && (m->power > 2)) {
            return vmap_resize(map, m->power - 1);
        }
        return VMAP_OK;
    }

    return VMAP_NO_KEY;
}

void vmap_delete(vmap* map) {
    size_t i, len = (1 << map->power);
    size_t key_size = map->type->key_size;
    size_t offset = key_size + map->padding;
    for (i = 0; i < len; ++i) {
        vmap_entry* e = map->entries[i];
        if (e == NULL) {
            continue;
        }
        if (e->flags & VMAP_DELETED) {
            vmap_free(e);
            continue;
        }
        if (e->flags & VMAP_EMPTY) {
            vmap_free(e);
            continue;
        }
        vmap_key_free(map, e->data);
        vmap_value_free(map, e->data + offset);
        vmap_free(e);
    }
    vmap_free(map->type);
    vmap_free(map);
}

static int vmap_resize(vmap** map, uint64_t new_power) {
    vmap* m = *map;
    uint64_t i, len = (1 << m->power);
    vmap* new_map = vmap_new_with_cap(m->type, new_power, m->padding);
    uint64_t (*hash_fn)(const void* key) = m->type->hash;
    uint64_t new_cap = (1 << new_map->power);
    if (new_map == NULL) {
        return VMAP_OOM;
    }
    for (i = 0; i < len; ++i) {
        vmap_entry* e = m->entries[i];
        uint64_t hash;
        if (e == NULL) {
            continue;
        }
        if (e->flags & VMAP_EMPTY) {
            vmap_free(e);
            continue;
        }
        if (e->flags & VMAP_DELETED) {
            vmap_free(e);
            continue;
        }
        hash = hash_fn(e->data) & (new_cap - 1);
        while (1) {
            vmap_entry* new_e = new_map->entries[hash];
            if (new_e != NULL) {
                hash = (hash + 1) & (new_cap - 1);
                continue;
            }
            new_map->entries[hash] = e;
            break;
        }
    }
    new_map->numel = m->numel;
    new_map->numelplusdeleted = m->numel;
    free(m);
    *map = new_map;
    return VMAP_OK;
}

static vmap* vmap_new_with_cap(vmap_type* type, uint64_t power,
                               size_t padding) {
    vmap* map;
    size_t cap = (1 << power);
    size_t needed = (sizeof *map) + (cap * sizeof(vmap_entry*));
    map = vmap_malloc(needed);
    if (map == NULL) {
        return NULL;
    }
    memset(map, 0, needed);
    map->type = type;
    map->power = power;
    map->padding = padding;
    return map;
}

static vmap_entry* vmap_entry_new(vmap* map, void* key, void* value) {
    vmap_entry* e;
    size_t key_size = map->type->key_size;
    size_t value_size = map->type->value_size;
    size_t padding = map->padding;
    size_t needed = (sizeof *e) + key_size + value_size + padding;
    e = vmap_malloc(needed);
    if (e == NULL) {
        return NULL;
    }
    memset(e, 0, needed);
    e->flags |= VMAP_FULL;
    memcpy(e->data, key, key_size);
    memcpy(e->data + key_size + padding, value, value_size);
    return e;
}
