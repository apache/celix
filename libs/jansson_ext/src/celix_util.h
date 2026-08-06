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
#ifndef CELIX_CELIX_UTIL_H
#define CELIX_CELIX_UTIL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* ── Dynamic string buffer ─────────────────────────────────────────────── */

typedef struct celix_jansson_strbuf_t {
    char* data;
    size_t len;
    size_t cap;
} celix_jansson_strbuf_t;

static inline void celix_jansson_strbuf_init(celix_jansson_strbuf_t* sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static inline void celix_jansson_strbuf_free(celix_jansson_strbuf_t* sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

/** Append @p len bytes from @p s. Returns 0 on success, -1 on ENOMEM. */
int celix_jansson_strbuf_append(celix_jansson_strbuf_t* sb, const char* s, size_t len);

/** Append a printf-formatted string. Returns 0 on success, -1 on ENOMEM. */
int celix_jansson_strbuf_appendf(celix_jansson_strbuf_t* sb, const char* fmt, ...)
#ifdef __GNUC__
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/** Va_list variant of celix_jansson_strbuf_appendf. */
int celix_jansson_strbuf_vappendf(celix_jansson_strbuf_t* sb, const char* fmt, va_list ap);

/** Append a single character. */
static inline int celix_jansson_strbuf_appendc(celix_jansson_strbuf_t* sb, char c) {
    return celix_jansson_strbuf_append(sb, &c, 1);
}

/** Append a NUL-terminated string. */
static inline int celix_jansson_strbuf_appends(celix_jansson_strbuf_t* sb, const char* s) {
    return celix_jansson_strbuf_append(sb, s, strlen(s));
}

/**
 * Detach the accumulated string.  Returns a malloc'd, NUL-terminated C
 * string; the strbuf is re-initialized to empty.  Returns NULL if empty.
 */
char* celix_jansson_strbuf_detach(celix_jansson_strbuf_t* sb);

/* ── Dynamic pointer array ──────────────────────────────────────────────── */

typedef struct celix_jansson_vec_t {
    void** items;
    size_t len;
    size_t cap;
} celix_jansson_vec_t;

static inline void celix_jansson_vec_init(celix_jansson_vec_t* v) {
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static inline void celix_jansson_vec_free(celix_jansson_vec_t* v) {
    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

int celix_jansson_vec_push(celix_jansson_vec_t* v, void* item);
void* celix_jansson_vec_pop(celix_jansson_vec_t* v);

static inline void* celix_jansson_vec_get(const celix_jansson_vec_t* v, size_t i) {
    return (i < v->len) ? v->items[i] : NULL;
}

static inline size_t celix_jansson_vec_size(const celix_jansson_vec_t* v) { return v->len; }

#endif /* CELIX_CELIX_UTIL_H */
