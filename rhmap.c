#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rhmap.h"

#define BUCKET_EMPTY(bucket) ((bucket)->dist == 0)

// valid for cap > 0
#define SET_CAP(map, cap_) do { (map)->cap = (cap_); (map)->hash_mask = (cap_) - 1; } while (0)
#define ZERO_CAP(map) do { (map)->cap = 0; (map)->hash_mask = 0; } while(0)

bool rhmap_init(RhMap* map, size_t cap, uint32_t (*hash)(void*), bool (*eq)(void*, void*)) {
    map->eq = eq;
    map->hash = hash;

    map->len = 0;
    if (cap == 0) cap = 16;
    SET_CAP(map, cap);

    map->buckets = calloc(cap, sizeof(RhMapBucket));
    map->keys = calloc(cap, sizeof(void*));
    map->values = calloc(cap, sizeof(void*));

    if (map->buckets == NULL) {
        map->cap = 0;
        // free(NULL) is valid
        free(map->buckets);
        free(map->keys);
        free(map->values);

        return false;
    }

    return true;
}

void rhmap_deinit(RhMap* map) {
    free(map->buckets);
    free(map->keys);
    free(map->values);
    map->buckets = map->keys = map->values = NULL;
    map->len = 0;
    ZERO_CAP(map);
}

static ssize_t lookup(const RhMap* map, void* key, uint32_t hash) {
    RhMapBucket* buckets = map->buckets;

    uint32_t dist = 1;
    for (;;) {
        size_t idx = (hash + dist - 1) & map->hash_mask;
        RhMapBucket* bucket = &buckets[idx];

        if (BUCKET_EMPTY(bucket)) return -1;
        if (bucket->hash == hash && map->eq(key, map->keys[idx])) {
            return idx;
        }
        // if bucket is richer than what we're looking for,
        // not found
        if (bucket->dist < dist) return -1;

        dist += 1;
    }
}

static ssize_t lookup_insert(const RhMap* map, void* key, void* value, uint32_t hash, void** old_value) {
    // temporary entry holding object to be inserted
    RhMapBucket entry;
    entry.dist = 1;
    entry.hash = hash;

    RhMapBucket* buckets = map->buckets;
    for (;;) {
        size_t idx = (hash + entry.dist - 1) & map->hash_mask;
        RhMapBucket* bucket = &buckets[idx];

        // insert in empty slot
        if (BUCKET_EMPTY(bucket)) {
            memcpy(bucket, &entry, sizeof(RhMapBucket));
            map->keys[idx] = key; map->values[idx] = value;
            return idx;
        }

        // entry already exists, replace
        if (bucket->hash == hash && map->eq(key, map->keys[idx])) {
            if (old_value != NULL)
                *old_value = map->values[idx];
            map->values[idx] = value;
            return idx;
        }

        // insert into richer slot
        if (bucket->dist < entry.dist) {
            RhMapBucket tmp;
            void *tmp_key, *tmp_value;

            memcpy(&tmp, bucket, sizeof(RhMapBucket));
            memcpy(bucket, &entry, sizeof(RhMapBucket));
            memcpy(&entry, &tmp, sizeof(RhMapBucket));

            tmp_key = map->keys[idx]; tmp_value = map->values[idx];
            map->keys[idx] = key; map->values[idx] = value;
            key=tmp_key; value=tmp_value;
        }

        entry.dist += 1;
    }
}

static ssize_t insert(const RhMap* map, void* key, void* value, uint32_t hash) {
    // temporary entry holding object to be inserted
    RhMapBucket entry;
    entry.dist = 1;
    entry.hash = hash;

    RhMapBucket* buckets = map->buckets;
    for (;;) {
        size_t idx = (hash + entry.dist - 1) & map->hash_mask;
        RhMapBucket* bucket = &buckets[idx];

        // insert in empty slot
        if (BUCKET_EMPTY(bucket)) {
            memcpy(bucket, &entry, sizeof(RhMapBucket));
            map->keys[idx] = key; map->values[idx] = value;
            return idx;
        }

        // insert into richer slot
        if (bucket->dist < entry.dist) {
            RhMapBucket tmp;
            void *tmp_key, *tmp_value;

            memcpy(&tmp, bucket, sizeof(RhMapBucket));
            memcpy(bucket, &entry, sizeof(RhMapBucket));
            memcpy(&entry, &tmp, sizeof(RhMapBucket));

            tmp_key = map->keys[idx]; tmp_value = map->values[idx];
            map->keys[idx] = key; map->values[idx] = value;
            key=tmp_key; value=tmp_value;
        }

        entry.dist += 1;
    }
}

#include <stdio.h>
// returns the old value, or NULL
void* rhmap_set(RhMap* map, void* key, void* value) {
    uint32_t hash = map->hash(key);
    
    // grow if % filled > load factor
    if (map->len > map->cap * 2/3) {
        if (!rhmap_grow(map, map->cap * 2)) {
            return NULL;
        }
    }

    void* old_value = NULL;
    lookup_insert(map, key, value, hash, &old_value);
    map->len += 1;
    return old_value;
}

bool rhmap_has(const RhMap* map, void* key) {
    uint32_t hash = map->hash(key);
    ssize_t iter = lookup(map, key, hash);
    return iter != -1;
}

void* rhmap_get(const RhMap* map, void* key) {
    uint32_t hash = map->hash(key);
    ssize_t iter = lookup(map, key, hash);
    if (iter == -1) return NULL;
    return map->values[iter];
}

void* rhmap_del(RhMap* map, void* key) {
    uint32_t hash = map->hash(key);
    ssize_t idx = lookup(map, key, hash);
    if (idx == -1) return NULL;

    void* value = map->values[idx];

    RhMapBucket* buckets = map->buckets;
    for (;;) {
        // current idx = to be discarded
        uint32_t next_idx = (idx + 1) & (map->hash_mask);

        if (BUCKET_EMPTY(&buckets[next_idx]) || buckets[next_idx].dist == 1) {
            buckets[idx].dist = 0; // clear current bucket; values and keys dont need to be overwritten
            break;
        }

        // copy next bucket to current bucket
        memcpy(&buckets[idx], &buckets[next_idx], sizeof(RhMapBucket)); 
        map->keys[idx] = map->keys[next_idx];
        map->values[idx] = map->values[next_idx];
        buckets[idx].dist -= 1;

        idx = next_idx;
    }

    map->len -= 1;
    return value;
}

bool rhmap_grow(RhMap* map, size_t cap) {
    RhMapBucket* old_buckets = map->buckets;
    void** old_keys = map->keys;
    void** old_values = map->values;
    uint32_t old_cap = map->cap;

    map->buckets = calloc(cap, sizeof(RhMapBucket));
    map->keys = calloc(cap, sizeof(void*));
    map->values = calloc(cap, sizeof(void*));
    if (map->buckets == NULL || map->keys == NULL || map->values == NULL) {
        free(map->buckets);
        free(map->keys);
        free(map->values);
        map->buckets = old_buckets;
        map->keys = old_keys;
        map->values = old_values;
        return false;
    }

    SET_CAP(map, cap);

    for (size_t i = 0; i < old_cap; i++) {
        if (!BUCKET_EMPTY(&old_buckets[i])) {
            insert(map, old_keys[i], old_values[i], old_buckets[i].hash);
        }
    }

    free(old_buckets);
    free(old_keys);
    free(old_values);

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