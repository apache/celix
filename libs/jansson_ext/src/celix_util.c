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
#include "celix_util.h"
#include <stdio.h>

/* ── String buffer ────────────────────────────────────────────────────── */

int celix_jansson_strbuf_append(celix_jansson_strbuf_t* sb, const char* s, size_t len) {
    if (len == 0)
        return 0;
    size_t needed = sb->len + len + 1; /* +1 for NUL */
    if (needed > sb->cap) {
        size_t new_cap = sb->cap ? sb->cap * 2 : 64;
        while (new_cap < needed)
            new_cap *= 2;
        char* new_data = (char*)realloc(sb->data, new_cap);
        if (!new_data)
            return -1;
        sb->data = new_data;
        sb->cap = new_cap;
    }
    memcpy(sb->data + sb->len, s, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
    return 0;
}

int celix_jansson_strbuf_appendf(celix_jansson_strbuf_t* sb, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = celix_jansson_strbuf_vappendf(sb, fmt, ap);
    va_end(ap);
    return r;
}

int celix_jansson_strbuf_vappendf(celix_jansson_strbuf_t* sb, const char* fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (needed < 0)
        return -1;

    size_t total_needed = sb->len + (size_t)needed + 1;
    if (total_needed > sb->cap) {
        size_t new_cap = sb->cap ? sb->cap * 2 : 64;
        while (new_cap < total_needed)
            new_cap *= 2;
        char* new_data = (char*)realloc(sb->data, new_cap);
        if (!new_data)
            return -1;
        sb->data = new_data;
        sb->cap = new_cap;
    }

    va_copy(ap2, ap);
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, ap2);
    va_end(ap2);
    sb->len += (size_t)needed;
    return 0;
}

char* celix_jansson_strbuf_detach(celix_jansson_strbuf_t* sb) {
    if (sb->len == 0)
        return NULL;
    char* s = sb->data;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    return s;
}

/* ── Dynamic pointer array ──────────────────────────────────────────────── */

int celix_jansson_vec_push(celix_jansson_vec_t* v, void* item) {
    if (v->len >= v->cap) {
        size_t new_cap = v->cap ? v->cap * 2 : 4;
        void** new_items = (void**)realloc(v->items, new_cap * sizeof(void*));
        if (!new_items)
            return -1;
        v->items = new_items;
        v->cap = new_cap;
    }
    v->items[v->len++] = item;
    return 0;
}

void* celix_jansson_vec_pop(celix_jansson_vec_t* v) {
    if (v->len == 0)
        return NULL;
    return v->items[--v->len];
}
