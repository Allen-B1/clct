#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <search.h>
#include <malloc.h>

#include <glib.h>

#include <stdint.h>
//#include <rhashmap.h>

#include <khash.h>
#include <kvec.h>
#include "arrlist.h"
#include "rhmap.h"
#include <cc_hashtable.h>

#define NS_PER_S 1000000000

#define B_TIME(type, op, block) do {\
    size_t mem = (size_t)mallinfo().uordblks; clock_t t = clock(); \
    block; \
    t = clock() - t; mem = (size_t)mallinfo().uordblks - mem; \
    printf(#type "," op ",%d,%.2f,%llu\n", n, ((double)t) / CLOCKS_PER_SEC * NS_PER_S / n, (unsigned long long)mem); \
} while(0)

#define INT_TO_PTR(i) (void*)(uintptr_t)(i)

#include <stdlib.h>

int n_values[] = {1 << 6, 1 << 7, 1 << 8, 1 << 9, (1<<9) + (1<<8), 1 << 10,
    1 << 14, 1 << 17, 1 << 18, 1 << 19, 1 << 20};
static char* strings[1 << 22];
void setup_strings() {
    for (int i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
        strings[i] = malloc(16);
        sprintf(strings[i], "%x", (unsigned)rand());
    }
}

void benchmark_kvec(int n) {
    kvec_t(int) arr;
    kv_init(arr);

    B_TIME(kvec_t, "append", {
        for (int i = 0; i < n; i++) {
            kv_push(int, arr, 1);
        }
    });

    kv_destroy(arr);
}

void benchmark_arrlist(int n) {
    ArrList arr;
    arrlist_init(&arr, 0);

    B_TIME(ArrList, "append", {
        for (int i =0 ; i<n;i++)
            arrlist_add(&arr, INT_TO_PTR(i));
    });

    arrlist_deinit(&arr);
}

void benchmark_garray(int n) {
    GArray* arr = g_array_new(FALSE, FALSE, sizeof(int));

    B_TIME(GArray, "append", {
        for (int i = 0; i < n; i++) {
            g_array_append_vals(arr, &i, 1);
        }
    });

    g_array_free(arr, TRUE);
}

void benchmark_rhmap(int n) {
    RhMap map;
    if (!rhmap_init(&map, 0, rhmap_djb2_str, rhmap_eq_str)) {
        printf("rhmap oom\n");
        return;
    }

    B_TIME(RhMap, "ins", {
        for (int i = 0; i < n; i++) {
            rhmap_set(&map, strings[i], INT_TO_PTR(i));
        }
    });

    B_TIME(RhMap, "get", {
        for (int i = 0; i < n; i++) {
            rhmap_get(&map, strings[i]);
        }
    });

    B_TIME(RhMap, "del", {
        for (int i = 0; i < n; i++) {
            rhmap_del(&map, strings[i]);
        }
    });

    rhmap_deinit(&map);
}

KHASH_MAP_INIT_STR(m32, int)
void benchmark_khash(int n) {
    khash_t(m32) *h = kh_init(m32);
    int absent;

    B_TIME(khash_t, "ins", {
        for (int i = 0; i < n; i++) {
            khiter_t iter = kh_put(m32, h, strings[i], &absent);
            kh_value(h, iter) = i;
        }
    });

    B_TIME(khash_t, "get", {
        for (int i = 0; i < n; i++) {
            khiter_t iter = kh_get(m32, h, strings[i]);
            kh_value(h, iter);
        }
    });

    B_TIME(khash_t, "del", {
        for (int i = 0; i < n; i++) {
            khiter_t iter = kh_get(m32, h, strings[i]);
            kh_del(m32, h, iter);
        }
    });

    kh_destroy(m32, h);
}

void benchmark_htab(int n) {
    struct hsearch_data* htab = calloc(1, sizeof(struct hsearch_data));
    hcreate_r(n, htab);

    ENTRY entry, *res;
    B_TIME(hsearch, "ins", {
        for (int i = 0; i < n; i++) {
            entry.key = strings[i];
            entry.data = i;
            hsearch_r(entry, ENTER, &res, htab);
        }
    });

    B_TIME(hsearch, "get", {
        for (int i = 0; i < n; i++) {
            entry.key = strings[i];
            hsearch_r(entry, FIND, &res, htab);
        }
    });

    hdestroy_r(htab);
}

void benchmark_ghashmap(int n) {
    GHashTable* map = g_hash_table_new(g_str_hash, g_str_equal);

    B_TIME(GHashTable, "ins", {
        for (int i = 0; i < n; i++) {
            g_hash_table_insert(map, strings[i], INT_TO_PTR(i));
        }
    });

    B_TIME(GHashTable, "get", {
        for (int i =0 ; i < n; i++) {
            g_hash_table_lookup(map, strings[i]);
        }
    });

    B_TIME(GHashTable, "del", {
        for (int i = 0; i < n; i++) {
            g_hash_table_remove(map, strings[i]);
        }
    });

    g_hash_table_destroy(map);
}

/*
void benchmark_rhashmap(int n) {
    rhashmap_t* map = rhashmap_create(0, RHM_NONCRYPTO);
    {
        B_START;
        for (int i = 0; i < n; i++)
            rhashmap_put(map, strings[i], strlen(strings[i]), &i);
        B_END;
        B_PRINT("rhashmap_t", "ins");
    }
    {
        B_START;
        for (int i = 0; i < n; i++)
            rhashmap_del(map, strings[i], strlen(strings[i]));
        B_END;
        B_PRINT("rhashmap_t", "del");
    }

    rhashmap_destroy(map);
}*/

void benchmark_cc_hashtable(int n) {
    CC_HashTable* table;
    cc_hashtable_new(&table);

    B_TIME(CC_HashTable, "ins", {
        for (int i =0; i<n;i++)
            cc_hashtable_add(table, strings[i], i);
    });

    B_TIME(CC_HashTable, "get", {
        for (int i =0 ; i < n; i++) {
            void* val;
            cc_hashtable_get(table, strings[i], &val);
        }
    });

    B_TIME(CC_HashTable, "del", {
        for (int i = 0; i<n; i++) {
            cc_hashtable_remove(table, strings[i], NULL);
        }
    });
}

void benchmark_cc_array(int n) {
    CC_Array* arr;
    cc_array_new(&arr);

    B_TIME(CC_Array, "append", {
        for (int i = 0; i < n; i++)
            cc_array_add(arr, strings[i]);
    });
}

// --- Benchmarking Memory Funcs --- //


int main() {

    setup_strings();

    time_t now = time(NULL);
    struct tm* info = localtime(&now);
    printf("date,%4u-%2u-%2u\n\n", (unsigned)(info->tm_year + 1900), (unsigned)(info->tm_mon+1), (unsigned)(info->tm_mday));

    #define DO(f) for (int i = 0; i < sizeof(n_values) / sizeof(n_values[i]); i++) { (f)(n_values[i]); }

    DO(benchmark_arrlist)
    DO(benchmark_kvec)
    DO(benchmark_garray)
    DO(benchmark_cc_array)

    // rhashmap_t is too slow :D
//    DO(benchmark_rhashmap)
    DO(benchmark_rhmap)
    DO(benchmark_khash)
    DO(benchmark_htab)
    DO(benchmark_ghashmap)
    DO(benchmark_cc_hashtable)

    return 0;
}