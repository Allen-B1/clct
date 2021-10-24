#ifndef _RHMAP_H
#define _RHMAP_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct RhMapBucket {
    uint32_t dist; // 0 = empty bucket
    uint32_t hash;
    void* key;
    void* value;
};

typedef struct {
    size_t len;
    size_t cap;
    uint32_t (*hash)(void*);
    bool (*eq)(void*, void*);
    struct RhMapBucket* data;
} RhMap;

bool rhmap_init(RhMap* map, size_t cap, uint32_t (*hash)(void*), bool (*eq)(void*, void*));
void* rhmap_set(RhMap* map, void* key, void* value) ;
void* rhmap_get(const RhMap* map, void* key);
bool rhmap_has(const RhMap* map, void* key);
void* rhmap_del(RhMap* map, void* key);
#define rhmap_foreach(map, keyvar, valvar, block) for (size_t _rh_i = 0; _rh_i < (map)->cap; _rh_i++) {\
    if ((map)->data[_rh_i].dist != 0) {\
        void* (keyvar) = (map)->data[_rh_i].key; \
        void* (valvar) = (map)->data[_rh_i].value; \
        block \
    }}
bool rhmap_grow(RhMap* map, size_t cap);
void rhmap_deinit(RhMap* map);

uint32_t rhmap_djb2_str(void* data);
uint32_t rhmap_djb2(void* data, size_t len);
uint32_t rhmap_djb2_int(void* data);

static bool rhmap_eq_str(void* s1, void* s2) {
    return strcmp(s1, s2) == 0;
}

static bool rhmap_eq_int(void* i1, void* i2) {
    return i1 == i2;
}

#endif