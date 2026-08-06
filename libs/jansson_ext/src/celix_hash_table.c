/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "celix_hash_table.h"
#include <stdlib.h>
#include <string.h>

/* FNV-1a hash */
static size_t hash_key(const char* key) {
    size_t h = 14695981039346656037ULL;
    for (const unsigned char* p = (const unsigned char*)key; *p; p++) {
        h ^= (size_t)*p;
        h *= 1099511628211ULL;
    }
    return h;
}

void celix_jansson_hash_table_init(celix_jansson_hash_table_t* ht) {
    ht->buckets = NULL;
    ht->count = 0;
    ht->cap = 0;
}

void celix_jansson_hash_table_destroy(celix_jansson_hash_table_t* ht,
                                      celix_jansson_hash_table_value_free_fn free_value) {
    celix_jansson_hash_table_clear(ht, free_value);
}

void celix_jansson_hash_table_clear(celix_jansson_hash_table_t* ht, celix_jansson_hash_table_value_free_fn free_value) {
    if (!ht->buckets)
        return;
    for (size_t i = 0; i < ht->cap; i++) {
        celix_jansson_hash_table_entry_t* e = &ht->buckets[i];
        if (e->key) {
            free(e->key);
            if (free_value && e->value)
                free_value(e->value);
        }
    }
    free(ht->buckets);
    ht->buckets = NULL;
    ht->count = 0;
    ht->cap = 0;
}

static int ht_grow(celix_jansson_hash_table_t* ht) {
    size_t new_cap = ht->cap ? ht->cap * 2 : 16;
    celix_jansson_hash_table_entry_t* new_buckets =
        (celix_jansson_hash_table_entry_t*)calloc(new_cap, sizeof(celix_jansson_hash_table_entry_t));
    if (!new_buckets)
        return -1;

    /* Rehash existing entries */
    for (size_t i = 0; i < ht->cap; i++) {
        celix_jansson_hash_table_entry_t* old = &ht->buckets[i];
        if (!old->key || old->used != 1)
            continue;

        size_t h = hash_key(old->key);
        for (size_t j = 0; j < new_cap; j++) {
            size_t idx = (h + j) % new_cap;
            if (!new_buckets[idx].key) {
                new_buckets[idx].key = old->key;
                new_buckets[idx].value = old->value;
                new_buckets[idx].used = 1;
                break;
            }
        }
    }

    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->cap = new_cap;
    return 0;
}

void* celix_jansson_hash_table_get(const celix_jansson_hash_table_t* ht, const char* key) {
    if (!ht->buckets || !key)
        return NULL;

    size_t h = hash_key(key);
    for (size_t j = 0; j < ht->cap; j++) {
        size_t idx = (h + j) % ht->cap;
        celix_jansson_hash_table_entry_t* e = &ht->buckets[idx];
        if (!e->key)
            return NULL; /* empty slot → not found */
        if (e->used == 1 && strcmp(e->key, key) == 0)
            return e->value;
    }
    return NULL;
}

int celix_jansson_hash_table_put(celix_jansson_hash_table_t* ht, const char* key, void* value) {
    if (!key)
        return -1;

    /* Check load factor (0.7 threshold) */
    if (ht->cap == 0 || (double)(ht->count + 1) / ht->cap > 0.7) {
        if (ht_grow(ht) != 0)
            return -1;
    }

    size_t h = hash_key(key);
    for (size_t j = 0; j < ht->cap; j++) {
        size_t idx = (h + j) % ht->cap;
        celix_jansson_hash_table_entry_t* e = &ht->buckets[idx];

        if (!e->key || e->used == 2) {
            /* Empty or tombstone — insert */
            char* kcopy = strdup(key);
            if (!kcopy)
                return -1;
            if (e->key)
                free(e->key);
            e->key = kcopy;
            e->value = value;
            e->used = 1;
            ht->count++;
            return 0;
        }

        if (e->used == 1 && strcmp(e->key, key) == 0) {
            /* Key exists — replace value (caller manages old value) */
            e->value = value;
            return 0;
        }
    }
    return -1; /* table full (shouldn't happen after grow) */
}

int celix_jansson_hash_table_remove(celix_jansson_hash_table_t* ht, const char* key) {
    if (!ht->buckets || !key)
        return -1;

    size_t h = hash_key(key);
    for (size_t j = 0; j < ht->cap; j++) {
        size_t idx = (h + j) % ht->cap;
        celix_jansson_hash_table_entry_t* e = &ht->buckets[idx];

        if (!e->key)
            return -1; /* not found */

        if (e->used == 1 && strcmp(e->key, key) == 0) {
            free(e->key);
            e->key = NULL;
            e->value = NULL;
            e->used = 2; /* tombstone */
            ht->count--;
            return 0;
        }
    }
    return -1;
}

void celix_jansson_hash_table_foreach(const celix_jansson_hash_table_t* ht,
                                      void (*cb)(const char* key, void* value, void* ud),
                                      void* ud) {
    if (!ht->buckets || !cb)
        return;
    for (size_t i = 0; i < ht->cap; i++) {
        celix_jansson_hash_table_entry_t* e = &ht->buckets[i];
        if (e->key && e->used == 1)
            cb(e->key, e->value, ud);
    }
}
