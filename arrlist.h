#ifndef _ARRLIST_H
#define _ARRLIST_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct ArrList {
    size_t cap;
    size_t len;
    void** data;
} ArrList;

void arrlist_init(ArrList* arr, size_t cap);
bool arrlist_grow(ArrList* arr, size_t cap);
bool arrlist_add(ArrList* arr, void* elem);
void* arrlist_get(const ArrList* arr, size_t idx);
bool arrlist_set(ArrList* arr, size_t idx, void* data);
void arrlist_clear(ArrList* dyarr);
void arrlist_pop(ArrList* dyarr);
void arrlist_deinit(ArrList* dyarr);

#endif