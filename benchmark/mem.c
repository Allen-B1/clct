/*
dkfkdmkfkasdfkmsakdfmkasbmajdgnjerwf

#define _GNU_SOURCE
#include <sys/cdefs.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <rhmap.h>

#define malloc real_malloc
#define calloc real_calloc
#define realloc real_realloc
#define free real_free
#include <dlfcn.h>
#undef malloc
#undef free
#undef calloc
#undef realloc

static void* (*real_malloc)(size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void (*real_free)(void*) = NULL;

static RhMap ptr_sizes;
static size_t total;
static bool in_custom; // whether this is 1) in init() 2) in a function; prevent inf recursion

static void init(void) {
    in_custom = true;

    #define SET_REAL(x) real_##x = dlsym(RTLD_NEXT, #x)
    SET_REAL(malloc);
    SET_REAL(realloc);
    SET_REAL(calloc);
    SET_REAL(free);
    #undef SET_REAL

    puts("hi?");

    if (real_malloc == NULL || real_calloc == NULL || real_realloc == NULL || real_free == NULL) {
        fprintf(stderr, "bmem: error in `dlsym`: %s\n", dlerror());
        exit(1);
    }

    rhmap_init(&ptr_sizes, 0, rhmap_djb2_int, rhmap_eq_int);

    in_custom = false;
}

#define HEADER(realexpr) do { \
    if (in_custom) return realexpr; \
    if (real_malloc == NULL) init(); \
    in_custom = true; \
} while(0)

#define END in_custom = false

extern void* malloc(size_t size) {
    HEADER(real_malloc(size));

    void* ptr = real_malloc(size);
    if (ptr == NULL) return NULL;

    rhmap_set(&ptr_sizes, ptr, (void*)(uintptr_t)size);
    total += size;

    END;
    return ptr;
}

extern void* calloc(size_t size, size_t size2) {
    HEADER(real_calloc(size, size2));

    void* ptr = real_calloc(size, size2);
    if (ptr == NULL) { 
        END;
        return NULL;
    }

    rhmap_set(&ptr_sizes, ptr, (void*)(uintptr_t)(size2*size));
    total += (size2*size);

    END;
    return ptr;
}

extern void* realloc(void* ptr, size_t size) {
    HEADER(real_realloc(ptr, size));

    ptr = real_realloc(ptr, size);
    if (ptr == NULL) { 
        END;
        return NULL;
    }

    size_t old_size = (uintptr_t)rhmap_get(&ptr_sizes, ptr);
    rhmap_set(&ptr_sizes, ptr, (void*)(uintptr_t)size);
    total += size - old_size;

    END;
    return ptr;
}

extern void free(void* ptr) 
{
    HEADER(real_free(ptr));

    real_free(ptr);

    size_t old_size = (uintptr_t)rhmap_get(&ptr_sizes, ptr);
    rhmap_set(&ptr_sizes, ptr, (void*)0);
    total -= old_size;

    END;
}
*/