/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TTABLE_H_
#define TTABLE_H_

#include <stdbool.h>
#include <stddef.h> // NULL
#include <stdint.h>
#include <string.h> // memset

#include "callbacks.h"
#include "infrastructure.h"

typedef struct
{
    void *key;
    void *value;
} TTable_entry;

typedef bool TTableEntryCompareFn(TTable_entry entry, const void *value);
typedef void TTableEntryRemoveCallback(TTable_entry *entry);

typedef struct
{
    uint32_t count;
    TTable_entry *entries;
    TTableEntryRemoveCallback *entry_remove_callback;
    TTableEntryCompareFn *key_compare_function;
    uint32_t size;
} TTable;

// Only one of these functions will be used for the given TTable depending on what TTable_entry's key is.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static bool ttable_compare_key_pointer(TTable_entry entry, const void *value)
{
    return entry.key == value;
}

static bool ttable_compare_key_string(TTable_entry entry, const void *value)
{
    return strcmp(entry.key, value) == 0;
}

static bool ttable_compare_key_uint32(TTable_entry entry, const void *value)
{
    return *(uint32_t *)entry.key == *(uint32_t *)value;
}
#pragma GCC diagnostic pop

static inline TTable *ttable_create(uint32_t entries_max, TTableEntryRemoveCallback *entry_remove_callback,
                                    TTableEntryCompareFn *key_compare_function)
{
    size_t ttable_size = entries_max * sizeof(TTable_entry);
    void *ttable_entries = tlib_mallocz(ttable_size);

    TTable *ttable = tlib_mallocz(sizeof(TTable));
    ttable->count = 0;
    ttable->entries = ttable_entries;
    ttable->entry_remove_callback = entry_remove_callback;
    ttable->key_compare_function = key_compare_function;
    ttable->size = entries_max;
    return ttable;
}

static inline void ttable_insert(TTable *ttable, void *key, void *value)
{
    tlib_assert(ttable->count < ttable->size);

    uint32_t first_free_entry_id = ttable->count;
    ttable->entries[first_free_entry_id].key = key;
    ttable->entries[first_free_entry_id].value = value;

    ttable->count++;
}

static inline TTable_entry *ttable_lookup_custom(TTable *ttable, TTableEntryCompareFn *entry_compare_function,
                                                 const void *compare_value)
{
    int i;
    for (i = 0; i < ttable->count; i++) {
        TTable_entry *entry = &ttable->entries[i];
        if (entry_compare_function(*entry, compare_value)) {
            return entry;
        }
    }
    return NULL;
}

static inline TTable_entry *ttable_lookup(TTable *ttable, void *key)
{
    return ttable_lookup_custom(ttable, ttable->key_compare_function, key);
}

static inline void *ttable_lookup_value_eq(TTable *ttable, void *key)
{
    TTable_entry *entry = ttable_lookup(ttable, key);
    if (entry) {
        return entry->value;
    }
    return NULL;
}

static inline bool ttable_insert_check(TTable *ttable, void *key, void *value)
{
    if (ttable_lookup(ttable, key)) {
        return false;
    }

    ttable_insert(ttable, key, value);
    return true;
}

static inline void ttable_remove(TTable *ttable)
{
    if (ttable->entry_remove_callback) {
        int i;
        for (i = 0; i < ttable->count; i++) {
            ttable->entry_remove_callback(&ttable->entries[i]);
        }
    }

    tlib_free(ttable->entries);
    tlib_free(ttable);
}

#endif // TTABLE_H_
