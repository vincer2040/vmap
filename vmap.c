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

#define vmap_h1(hash) ((hash) >> 2)
#define vmap_flags(hash) ((((hash) << 62) | 0) >> 62)

struct vmap {
    uint64_t numel;
    uint64_t numelplusdeleted;
    uint64_t power;
    size_t entry_padding;
    size_t metadata_padding;
    size_t entry_size;
    uint64_t* metadata;
    unsigned char* entries;
    vmap_type* type;
    unsigned char data[];
};

#define vmap_padding(key_size)                                                 \
    ((sizeof(void*) - (((key_size)) % sizeof(void*))) & (sizeof(void*) - 1))

static vmap* vmap_new_with_cap(vmap_type* type, uint64_t power, size_t entry_padding, size_t entry_size);
static int vmap_resize(vmap** map, uint64_t new_power);

vmap* vmap_new(vmap_type* type) {
    vmap* map;
    size_t padding, metadata_padding, entry_size, metadata_size, needed,
        initial_cap;
    if (type->key_size == 0) {
        return NULL;
    }
    if (type->value_size == 0) {
        return NULL;
    }
    if (type->hash == NULL) {
        return NULL;
    }
    initial_cap = 1 << VMAP_INITIAL_POWER;
    padding = vmap_padding(type->key_size);
    entry_size = type->key_size + type->value_size + padding;
    metadata_size = sizeof(uint64_t) * initial_cap;
    metadata_padding = vmap_padding(metadata_size);
    needed = (sizeof *map) + (metadata_size + metadata_padding) +
             (entry_size * initial_cap);
    map = vmap_malloc(needed);
    if (map == NULL) {
        return NULL;
    }
    memset(map, 0, needed);
    map->type = type;
    map->entry_padding = padding;
    map->metadata_padding = metadata_padding;
    map->entry_size = entry_size;
    map->metadata = (uint64_t*)(map->data);
    map->entries = map->data + (metadata_size + metadata_padding);
    map->power = VMAP_INITIAL_POWER;
    return map;
}

int vmap_insert(vmap** map, void* key, void* value) {
    vmap* m = *map;
    uint64_t hash = m->type->hash(key);
    uint64_t capacity = 1 << m->power;
    uint64_t h1 = vmap_h1(hash);
    uint64_t slot = h1 & (capacity - 1);
    size_t key_size = m->type->key_size;
    size_t value_size = m->type->value_size;
    size_t value_offset = key_size + m->entry_padding;
    double load;
    if (m->numelplusdeleted >= capacity) {
        return VMAP_OOM;
    }
    while (1) {
        uint64_t data = m->metadata[slot];
        uint64_t data_h1 = vmap_h1(data);
        uint8_t flags = vmap_flags(data);
        if (flags == VMAP_DELETED) {
            slot = (slot + 1) & (capacity - 1);
        }
        if (flags == VMAP_FULL) {
            void* cur_key;
            if (data_h1 == h1) {
                int cmp;
                cur_key = m->entries + (m->entry_size * slot);
                cmp = vmap_key_cmp(m, key, cur_key);
                if (cmp != 0) {
                    slot = (slot + 1) & (capacity - 1);
                    continue;
                }
                vmap_key_free(m, key);
                memcpy(m->entries + (m->entry_size * slot) + value_offset,
                       value, value_size);
                break;
            }
            slot = (slot + 1) & (capacity - 1);
            continue;
        }
        m->metadata[slot] = (h1 << 2) | VMAP_FULL;
        memcpy(m->entries + (m->entry_size * slot), key, key_size);
        memcpy(m->entries + (m->entry_size * slot) + value_offset, value,
               value_size);
        m->numelplusdeleted++;
        m->numel++;
        load = (double)m->numelplusdeleted / (double)capacity;
        if (load > VMAP_MAX_LOAD) {
            return vmap_resize(map, m->power + 1);
        }
        break;
    }
    return VMAP_OK;
}

const void* vmap_find(vmap* map, const void* key) {
    uint64_t hash = map->type->hash(key);
    uint64_t capacity = 1 << map->power;
    uint64_t h1 = vmap_h1(hash);
    uint64_t slot = h1 & (capacity - 1);
    size_t key_size = map->type->key_size;
    size_t value_offset = key_size + map->entry_padding;
    while (1) {
        uint64_t data = map->metadata[slot];
        uint64_t data_h1 = vmap_h1(data);
        uint8_t flags = vmap_flags(data);
        if (flags == VMAP_DELETED) {
            slot = (slot + 1) & (capacity - 1);
            continue;
        }
        if (flags == VMAP_EMPTY) {
            break;
        }
        if (data_h1 == h1) {
            void* cur_key = map->entries + (map->entry_size * slot);
            int cmp = vmap_key_cmp(map, key, cur_key);
            if (cmp == 0) {
                return map->entries + (map->entry_size * slot) + value_offset;
            }
        }
        slot = (slot + 1) & (capacity - 1);
    }
    return NULL;
}

int vmap_erase(vmap** map, const void* key) {
    vmap* m = *map;
    uint64_t hash = m->type->hash(key);
    uint64_t capacity = 1 << m->power;
    uint64_t h1 = vmap_h1(hash);
    uint64_t slot = h1 & (capacity - 1);
    size_t key_size = m->type->key_size;
    size_t value_offset = key_size + m->entry_padding;
    double load;
    while (1) {
        uint64_t data = m->metadata[slot];
        uint64_t data_h1 = vmap_h1(data);
        uint8_t flags = vmap_flags(data);
        if (flags == VMAP_DELETED) {
            slot = (slot + 1) & (capacity - 1);
            continue;
        }
        if (flags == VMAP_EMPTY) {
            break;
        }
        if (data_h1 == h1) {
            void* cur_key = m->entries + (m->entry_size * slot);
            int cmp = vmap_key_cmp(m, key, cur_key);
            if (cmp == 0) {
                vmap_key_free(m, cur_key);
                vmap_value_free(m, m->entries + (m->entry_size * slot) + value_offset);
                m->metadata[slot] = VMAP_DELETED;
                m->numel--;
                load = (double)m->numel / (double)capacity;
                if ((load < VMAP_MIN_LOAD) && (m->power > VMAP_INITIAL_POWER)) {
                    return vmap_resize(map, m->power - 1);
                }
                return VMAP_OK;
            }
        }
        slot = (slot + 1) & (capacity - 1);
    }
    return VMAP_NO_KEY;
}

void vmap_delete(vmap* map) {
    vmap_free(map->type);
    vmap_free(map);
}

static int vmap_resize(vmap** map, uint64_t new_power) {
    vmap* m = *map;
    size_t i, len = 1 << m->power;
    vmap* new_map = vmap_new_with_cap(m->type, new_power, m->entry_padding, m->entry_size);
    size_t entry_size = m->entry_size;
    uint64_t new_cap = 1 << new_power;
    if (new_map == NULL) {
        return VMAP_OOM;
    }
    for (i = 0; i < len; ++i) {
        uint64_t data = m->metadata[i];
        uint8_t flags = vmap_flags(data);
        uint64_t h1 = vmap_h1(data);
        uint64_t slot;
        void* entry;
        if (flags == VMAP_DELETED) {
            continue;
        }
        if (flags == VMAP_EMPTY) {
            continue;
        }
        entry = m->entries + (entry_size * i);
        slot = h1 & (new_cap - 1);
        while (1) {
            uint64_t new_data = new_map->metadata[slot];
            uint8_t new_flags = vmap_flags(new_data);
            if (new_flags == VMAP_FULL) {
                slot = (slot + 1) & (new_cap - 1);
                continue;
            }
            memcpy(new_map->entries + (slot * entry_size), entry, entry_size);
            new_map->metadata[slot] = (h1 << 2) | VMAP_FULL;
            break;
        }
    }
    new_map->numelplusdeleted = m->numel;
    new_map->numel = m->numel;
    vmap_free(m);
    *map = new_map;
    return VMAP_OK;
}

static vmap* vmap_new_with_cap(vmap_type* type, uint64_t power, size_t entry_padding, size_t entry_size) {
    vmap* map;
    size_t new_cap = 1 << power;
    size_t metadata_size = sizeof(uint64_t) * new_cap;
    size_t metadata_padding = vmap_padding(metadata_size);
    size_t needed = (sizeof *map) + metadata_size + metadata_padding + (entry_size * new_cap);
    map = vmap_malloc(needed);
    if (map == NULL) {
        return NULL;
    }
    memset(map, 0, needed);
    map->type = type;
    map->entry_padding = entry_padding;
    map->metadata_padding = metadata_padding;
    map->entry_size = entry_size;
    map->metadata = (uint64_t*)(map->data);
    map->entries = map->data + metadata_size + metadata_padding;
    map->power = power;
    return map;
}
