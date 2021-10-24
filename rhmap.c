#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rhmap.h"

#define BUCKET_EMPTY(bucket) ((bucket)->dist == 0)

bool rhmap_init(RhMap* map, size_t cap, uint32_t (*hash)(void*), bool (*eq)(void*, void*)) {
    if (cap == 0) cap = 16;

    map->eq = eq;
    map->hash = hash;
    map->data = calloc(sizeof(struct RhMapBucket), cap);
    map->len = 0;
    if (map->data == NULL) {
        map->cap = 0;
        return false;
    }
    map->cap = cap;
    return true;
}

void rhmap_deinit(RhMap* map) {
    free(map->data);
    map->data = NULL;
    map->len = 0;
    map->cap = 0;
}

static struct RhMapBucket* lookup(const RhMap* map, void* key, uint32_t hash) {
    struct RhMapBucket* buckets = map->data;

    uint32_t idx = hash & (map->cap - 1);
    uint32_t dist = 1;
    for (;;) {
        struct RhMapBucket* bucket = &buckets[(idx + dist - 1) & (map->cap - 1)];
        if (BUCKET_EMPTY(bucket)) return NULL;
        if (bucket->hash == hash && map->eq(key, bucket->key)) {
            return bucket;
        }
        // if bucket is richer than what we're looking for,
        // not found
        if (bucket->dist < dist) {
            return NULL;
        }

        dist += 1;
    }
}

static struct RhMapBucket* insert(const RhMap* map, void* key, void* value, uint32_t hash) {
    struct RhMapBucket entry;
    entry.dist = 1;
    entry.hash = hash;
    entry.key = key;
    entry.value = value;

    struct RhMapBucket* buckets = map->data;
    uint32_t idx = hash & (map->cap - 1);
    for (;;) {
        struct RhMapBucket* bucket = &buckets[(idx + entry.dist - 1) & (map->cap - 1)];

        if (BUCKET_EMPTY(bucket)) {
            memcpy(bucket, &entry, sizeof(struct RhMapBucket));
            return bucket;
        }

        // take from rich, give to poor
        if (bucket->dist < entry.dist) {
            struct RhMapBucket tmp;
            memcpy(&tmp, bucket, sizeof(struct RhMapBucket));
            memcpy(bucket, &entry, sizeof(struct RhMapBucket));
            memcpy(&entry, &tmp, sizeof(struct RhMapBucket));
        }

        entry.dist += 1;
    }
}

#include <stdio.h>
// returns the old value, or NULL
void* rhmap_set(RhMap* map, void* key, void* value) {
    uint32_t hash = map->hash(key);
    
    struct RhMapBucket* bucket = lookup(map, key, hash);
    if (bucket != NULL) {
        void* old = bucket->value;
        bucket->value = value;
        return old;
    }

    // load factor = 3/4
    if (map->len >= map->cap * 3/4) {
        if (!rhmap_grow(map, map->cap * 2)) {
            return NULL;
        }
    }

    insert(map, key, value, hash);
    map->len += 1;
    return NULL;
}

bool rhmap_has(const RhMap* map, void* key) {
    uint32_t hash = map->hash(key);
    struct RhMapBucket* bucket = lookup(map, key, hash);
    return bucket != NULL;
}

void* rhmap_get(const RhMap* map, void* key) {
    uint32_t hash = map->hash(key);
    struct RhMapBucket* bucket = lookup(map, key, hash);
    if (bucket != NULL) {
        return bucket->value;
    }
    return NULL;
}

void* rhmap_del(RhMap* map, void* key) {
    uint32_t hash = map->hash(key);
    struct RhMapBucket* bucket = lookup(map, key, hash);
    if (bucket == NULL) return NULL;
    void* value = bucket->value;

    struct RhMapBucket* buckets = (struct RhMapBucket*)map->data;
    uint32_t idx = bucket - buckets;
    for (;;) {
        uint32_t next_idx = (idx + 1) & (map->cap - 1);
        if (BUCKET_EMPTY(&buckets[next_idx]) || buckets[next_idx].dist == 1) {
            buckets[idx].dist = 0;
            break;
        }
        memcpy(&buckets[idx], &buckets[next_idx], sizeof(struct RhMapBucket));
        buckets[idx].dist -= 1;
        idx = next_idx;
    }

    map->len -= 1;
    return value;
}

bool rhmap_grow(RhMap* map, size_t cap) {
    struct RhMapBucket* old_data = map->data;
    uint32_t old_cap = map->cap;

    map->data = calloc(sizeof(struct RhMapBucket), cap);
    if (map->data == NULL) {
        return false;
    }
    map->cap = cap;

    for (size_t i = 0; i < old_cap; i++) {
        if (!BUCKET_EMPTY(&old_data[i]))
            insert(map, old_data[i].key, old_data[i].value, old_data[i].hash);
    }

    free(old_data); 
    return true;
}

uint32_t rhmap_djb2_str(void* data) {
    char* str = data;

    uint32_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

uint32_t rhmap_djb2(void* data, size_t len) {
    char* str = data;

    uint32_t hash = 5381;

    for (size_t i = 0; i < len; i++)
        hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */

    return hash;
}

uint32_t rhmap_djb2_int(void* data) {
    return rhmap_djb2(&data, sizeof(void*));
}