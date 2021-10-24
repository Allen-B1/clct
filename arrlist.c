#include "arrlist.h"

void arrlist_init(ArrList* arr, size_t cap) {
    arr->cap = cap;
    arr->len = 0;
    arr->data = malloc(sizeof(void*) * cap);
    if (arr->data == NULL) {
        arr->cap = 0;
    }
}

bool arrlist_grow(ArrList* arr, size_t cap) {
    if (arr->cap >= cap) {
        return true;
    }

    void* new_data = realloc(arr->data, sizeof(void*) * cap);
    if (new_data == NULL) {
        return false;
    }

    arr->data = new_data;
    arr->cap = cap;

    return true;
}

bool arrlist_add(ArrList* arr, void* elem) {
    if (arr->cap == arr->len) {
        size_t cap = arr->cap;
        cap = (cap + (cap >> 2) + 6) & ~(size_t)3;
        if (!arrlist_grow(arr, cap)) return false;
    }

    arr->data[arr->len] = elem;
    arr->len += 1;
    return true;
}

void* arrlist_get(const ArrList* arr, size_t idx) {
    if (idx >= arr->len) {
        return NULL;
    }
    return arr->data[idx];
}

bool arrlist_set(ArrList* arr, size_t idx, void* elem) {
    if (idx >= arr->len) {
        return false;
    }
    arr->data[idx] = elem;
    return true;
}

void arrlist_pop(ArrList* arr) {
    arr->len -= 1;
}

void arrlist_clear(ArrList* arr) {
    arr->len = 0;
}

void arrlist_deinit(ArrList* arr) {
    free(arr->data);
    arr->data = NULL;
    arr->len = 0;
    arr->cap = 0;
}