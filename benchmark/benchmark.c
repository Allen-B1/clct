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

#define B_START clock_t t = clock(); size_t mem = (size_t)mallinfo().uordblks
#define B_END t = clock() - t; mem = (size_t)mallinfo().uordblks - mem
#define B_PRINT(type, op) printf(type "," op ",%d,%.2f,%u\n", n, ((double)t) / CLOCKS_PER_SEC * NS_PER_S / n, (unsigned)mem)

#define INT_TO_PTR(i) (void*)(uintptr_t)(i)

int n_values[] = {1 << 6, 1 << 7, 1 << 8, 1 << 9, (1<<9) + (1<<8), 1 << 10,
    1 << 14, 1 << 17, 1 << 18, 1 << 19, 1 << 20};
static char* strings[1 << 22];
void setup_strings() {
    for (int i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
        strings[i] = malloc(16);
        sprintf(strings[i], "%d", i);
    }
}


void benchmark_kvec(int n) {
    kvec_t(int) arr;
    kv_init(arr);

    B_START;
    for (int i = 0; i < n; i++) {
        kv_push(int, arr, 1);
    }
    B_END;

    kv_destroy(arr);

    B_PRINT("kvec_t", "add");
}

void benchmark_arrlist(int n) {
    ArrList arr;
    arrlist_init(&arr, 0);

    B_START;

    for (int i = 0; i < n; i++) {
        arrlist_add(&arr, INT_TO_PTR(i));
    }

    B_END;

    arrlist_deinit(&arr);

    B_PRINT("ArrList", "add");
}

void benchmark_garray(int n) {
    GArray* arr = g_array_new(FALSE, FALSE, sizeof(int));

    B_START;
    for (int i = 0; i < n; i++) {
        g_array_append_vals(arr, &i, 1);
    }
    B_END;
    B_PRINT("GArray", "add");

    g_array_free(arr, TRUE);
}

void benchmark_rhmap(int n) {
    RhMap map;
    if (!rhmap_init(&map, 0, rhmap_djb2_str, rhmap_eq_str)) {
        printf("rhmap oom\n");
        return;
    }

    {
        B_START;
        for (int i = 0; i < n; i++) {
            rhmap_set(&map, strings[i], INT_TO_PTR(i));
        }
        B_END;
        B_PRINT("RhMap", "ins");
    }
    {
        B_START;
        for (int i = 0; i < n; i++) {
            rhmap_del(&map, strings[i]);
        }
        B_END;
        B_PRINT("RhMap", "del");
    }

    rhmap_deinit(&map);
}

KHASH_MAP_INIT_STR(m32, int)
void benchmark_khash(int n) {
    khash_t(m32) *h = kh_init(m32);
    int absent;

    {
        B_START;
        for (int i = 0; i < n; i++) {
            khiter_t iter = kh_put(m32, h, strings[i], &absent);
            kh_value(h, iter) = i;
        }
        B_END;
        B_PRINT("khash_t", "ins");
    }

    {
        B_START;
        for (int i = 0; i < n; i++) {
            khiter_t iter = kh_get(m32, h, strings[i]);
            kh_del(m32, h, iter);
        }
        B_END;
        B_PRINT("khash_t", "del");
    }
}

void benchmark_htab(int n) {
    struct hsearch_data* htab = calloc(1, sizeof(struct hsearch_data));
    hcreate_r(n, htab);

    B_START;
    for (int i = 0; i < n; i++) {
        ENTRY entry, *res;
        entry.key = strings[i];
        entry.data = i;
        hsearch_r(entry, ENTER, &res, htab);
    }
    B_END;
    B_PRINT("hsearch", "ins");

    hdestroy_r(htab);
}

void benchmark_ghashmap(int n) {
    GHashTable* map = g_hash_table_new(g_str_hash, g_str_equal);

    {
        B_START;
        for (int i = 0; i < n; i++) {
            g_hash_table_insert(map, strings[i], INT_TO_PTR(i));
        }
        B_END;
        B_PRINT("GHashTable", "ins");
    }

    {
        B_START;
        for (int i = 0; i < n; i++) {
            g_hash_table_remove(map, strings[i]);
        }
        B_END;
        B_PRINT("GHashTable", "del");
    }


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

    {
        B_START;
        for (int i =0; i<n;i++)
            cc_hashtable_add(table, strings[i], i);
        B_END;
        B_PRINT("CC_HashTable", "ins");
    }

    {
        B_START;
        for (int i = 0; i<n; i++) {
            cc_hashtable_remove(table, strings[i], NULL);
        }
        B_END;
        B_PRINT("CC_HashTable", "del");
    }
}

void benchmark_cc_array(int n) {
    CC_Array* arr;
    cc_array_new(&arr);

    B_START;
    for (int i = 0; i < n; i++)
        cc_array_add(arr, strings[i]);
    B_END;
    B_PRINT("CC_Array", "add");
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