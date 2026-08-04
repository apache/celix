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
#ifndef CELIX_CELIX_HASH_TABLE_H
#define CELIX_CELIX_HASH_TABLE_H

#include <stddef.h>

typedef void (*celix_jansson_hash_table_value_free_fn)(void* value);

typedef struct celix_jansson_hash_table_entry_t {
    char* key;
    void* value;
    int used; /* 0 = empty, 1 = occupied, 2 = tombstone */
} celix_jansson_hash_table_entry_t;

typedef struct celix_jansson_hash_table_t {
    celix_jansson_hash_table_entry_t* buckets;
    size_t count;
    size_t cap;
} celix_jansson_hash_table_t;

void celix_jansson_hash_table_init(celix_jansson_hash_table_t* ht);
void celix_jansson_hash_table_destroy(celix_jansson_hash_table_t* ht,
                                      celix_jansson_hash_table_value_free_fn free_value);
void celix_jansson_hash_table_clear(celix_jansson_hash_table_t* ht, celix_jansson_hash_table_value_free_fn free_value);

/** Look up a key. Returns the value pointer, or NULL if not found. */
void* celix_jansson_hash_table_get(const celix_jansson_hash_table_t* ht, const char* key);

/**
 * Insert or replace a key-value pair.  The key is copied; the value pointer
 * is stored as-is (no ownership).  Returns 0 on success, -1 on ENOMEM.
 * Replacing an existing key does NOT free the old value — caller must do
 * that themselves before calling celix_jansson_hash_table_put if needed.
 */
int celix_jansson_hash_table_put(celix_jansson_hash_table_t* ht, const char* key, void* value);

/** Remove a key. Returns 0 if found and removed, -1 if not found. */
int celix_jansson_hash_table_remove(celix_jansson_hash_table_t* ht, const char* key);

/** Number of entries in the table. */
static inline size_t celix_jansson_hash_table_size(const celix_jansson_hash_table_t* ht) { return ht->count; }

/**
 * Iterate over all entries.  The callback receives key, value, and the
 * user_data pointer.
 */
void celix_jansson_hash_table_foreach(const celix_jansson_hash_table_t* ht,
                                      void (*cb)(const char* key, void* value, void* ud),
                                      void* ud);

#endif /* CELIX_CELIX_HASH_TABLE_H */
