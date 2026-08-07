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
#include "celix_jansson_pointer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static int is_digit(char c) { return c >= '0' && c <= '9'; }

static int ensure_cap(celix_json_pointer_t* p, size_t needed) {
    if (p->cap >= needed)
        return 0;
    size_t nc = p->cap ? p->cap * 2 : 8;
    while (nc < needed)
        nc *= 2;
    char** nt = (char**)realloc(p->tokens, nc * sizeof(char*));
    if (!nt)
        return -1;
    p->tokens = nt;
    p->cap = nc;
    return 0;
}

/* ── Lifecycle ─────────────────────────────────────────────────────── */

celix_json_pointer_t* celix_json_pointer_create(const char* ptr_str) {
    celix_json_pointer_t* p = (celix_json_pointer_t*)calloc(1, sizeof(*p));
    if (!p)
        return NULL;
    if (ptr_str && celix_json_pointer_init(p, ptr_str) != 0) {
        free(p);
        return NULL;
    }
    return p;
}

int celix_json_pointer_init(celix_json_pointer_t* p, const char* ptr_str) {
    if (!p)
        return -1;
    celix_json_pointer_clear(p);
    if (!ptr_str || *ptr_str == '\0')
        return 0;
    if (*ptr_str != '/')
        return -1;

    const char* s = ptr_str + 1;
    if (*s == '\0')
        return celix_json_pointer_push(p, "");

    int ret = -1;
    while (*s) {
        const char* tok_start = s;
        while (*s && *s != '/')
            s++;
        size_t tok_len = (size_t)(s - tok_start);

        char* decoded = (char*)malloc(tok_len + 1);
        if (!decoded)
            goto error;

        size_t di = 0;
        for (size_t si = 0; si < tok_len; si++) {
            if (tok_start[si] == '~') {
                if (si + 1 >= tok_len) {
                    free(decoded);
                    goto error;
                }
                if (tok_start[si + 1] == '0') {
                    decoded[di++] = '~';
                    si++;
                } else if (tok_start[si + 1] == '1') {
                    decoded[di++] = '/';
                    si++;
                } else {
                    free(decoded);
                    goto error;
                }
            } else {
                decoded[di++] = tok_start[si];
            }
        }
        decoded[di] = '\0';

        /* RFC 6901: array-index tokens must not have leading zeros */
        if (decoded[0] == '0' && decoded[1] != '\0') {
            bool all_digits = true;
            for (size_t j = 0; decoded[j]; j++)
                if (!is_digit(decoded[j])) {
                    all_digits = false;
                    break;
                }
            if (all_digits) {
                free(decoded);
                goto error;
            }
        }

        if (celix_json_pointer_push(p, decoded) != 0) {
            free(decoded);
            goto error;
        }
        free(decoded);

        if (*s == '/') {
            s++;
            if (*s == '\0') { /* trailing slash → empty final token */
                if (celix_json_pointer_push(p, "") != 0)
                    goto error;
            }
        }
    }
    ret = 0;

error:
    if (ret != 0)
        celix_json_pointer_clear(p);
    return ret;
}

celix_json_pointer_t* celix_json_pointer_copy(const celix_json_pointer_t* src) {
    if (!src)
        return NULL;
    celix_json_pointer_t* dst = (celix_json_pointer_t*)calloc(1, sizeof(*dst));
    if (!dst)
        return NULL;
    if (ensure_cap(dst, src->len) != 0) {
        free(dst);
        return NULL;
    }
    for (size_t i = 0; i < src->len; i++) {
        dst->tokens[i] = strdup(src->tokens[i]);
        if (!dst->tokens[i]) {
            celix_json_pointer_destroy(dst);
            return NULL;
        }
        dst->len++;
    }
    return dst;
}

void celix_json_pointer_destroy(celix_json_pointer_t* ptr) {
    if (!ptr)
        return;
    celix_json_pointer_clear(ptr);
    free(ptr);
}

void celix_json_pointer_clear(celix_json_pointer_t* ptr) {
    if (!ptr)
        return;
    for (size_t i = 0; i < ptr->len; i++)
        free(ptr->tokens[i]);
    free(ptr->tokens);
    memset(ptr, 0, sizeof(*ptr));
}

/* ── Inspection ─────────────────────────────────────────────────────── */

size_t celix_json_pointer_depth(const celix_json_pointer_t* ptr) { return ptr ? ptr->len : 0; }

const char* celix_json_pointer_token(const celix_json_pointer_t* ptr, size_t idx) {
    return (ptr && idx < ptr->len) ? ptr->tokens[idx] : NULL;
}

/* ── Mutation ───────────────────────────────────────────────────────── */

int celix_json_pointer_push(celix_json_pointer_t* ptr, const char* token) {
    if (!ptr || !token || ensure_cap(ptr, ptr->len + 1) != 0)
        return -1;
    ptr->tokens[ptr->len] = strdup(token);
    if (!ptr->tokens[ptr->len])
        return -1;
    ptr->len++;
    return 0;
}

void celix_json_pointer_pop(celix_json_pointer_t* ptr) {
    if (!ptr || ptr->len == 0)
        return;
    ptr->len--;
    free(ptr->tokens[ptr->len]);
    ptr->tokens[ptr->len] = NULL;
}

/* ── Serialization ──────────────────────────────────────────────────── */

char* celix_json_pointer_to_string(const celix_json_pointer_t* ptr) {
    if (!ptr)
        return NULL;

    size_t total = 1;
    for (size_t i = 0; i < ptr->len; i++) {
        for (const char* c = ptr->tokens[i]; *c; c++)
            total += (*c == '~' || *c == '/') ? 2 : 1;
        if (i < ptr->len - 1)
            total++;
    }

    char* out = (char*)malloc(total + 1);
    if (!out)
        return NULL;
    char* w = out;
    if (ptr->len == 0) {
        *w++ = '/';
        *w = '\0';
    } else {
        for (size_t i = 0; i < ptr->len; i++) {
            *w++ = '/';
            for (const char* c = ptr->tokens[i]; *c; c++) {
                if (*c == '~') {
                    *w++ = '~';
                    *w++ = '0';
                } else if (*c == '/') {
                    *w++ = '~';
                    *w++ = '1';
                } else
                    *w++ = *c;
            }
        }
    }
    *w = '\0';
    return out;
}

/* ── Document access ────────────────────────────────────────────────── */

json_t* celix_json_pointer_get(json_t* doc, const celix_json_pointer_t* ptr) {
    if (!doc || !ptr)
        return NULL;
    json_t* cur = doc;
    for (size_t i = 0; i < ptr->len; i++) {
        if (json_is_object(cur)) {
            cur = json_object_get(cur, ptr->tokens[i]);
        } else if (json_is_array(cur)) {
            const char* tok = ptr->tokens[i];
            if (*tok == '-')
                return NULL;
            for (const char* c = tok; *c; c++)
                if (!is_digit(*c))
                    return NULL;
            size_t idx = (size_t)strtoul(tok, NULL, 10);
            cur = json_array_get(cur, idx);
        } else
            return NULL;
        if (!cur)
            return NULL;
    }
    return cur;
}

int celix_json_pointer_contains(json_t* doc, const celix_json_pointer_t* ptr) {
    return celix_json_pointer_get(doc, ptr) != NULL;
}

/* ── get_or_create ──────────────────────────────────────────────────── */

json_t* celix_json_pointer_get_or_create(json_t* doc, const celix_json_pointer_t* ptr) {
    if (!doc || !ptr || !(json_is_object(doc) || json_is_array(doc)))
        return NULL;

    json_t* cur = doc;
    for (size_t i = 0; i < ptr->len; i++) {
        const char* tok = ptr->tokens[i];
        bool is_last = (i == ptr->len - 1);

        if (json_is_object(cur)) {
            json_t* child = json_object_get(cur, tok);
            if (!child) {
                if (is_last) {
                    child = json_null();
                    json_object_set(cur, tok, child);
                    return child;
                }
                bool is_num = true;
                if (*tok == '\0')
                    is_num = false;
                for (const char* c = tok; *c; c++)
                    if (!is_digit(*c)) {
                        is_num = false;
                        break;
                    }
                if (is_num && tok[0] == '0' && tok[1] != '\0')
                    is_num = false;
                child = is_num ? json_array() : json_object();
                json_object_set_new(cur, tok, child);
            }
            cur = child;
        } else if (json_is_array(cur)) {
            if (strcmp(tok, "-") == 0) {
                if (is_last) {
                    json_incref(cur);
                    return cur;
                }
                return NULL;
            }
            for (const char* c = tok; *c; c++)
                if (!is_digit(*c))
                    return NULL;
            size_t idx = (size_t)strtoul(tok, NULL, 10);
            while (json_array_size(cur) <= idx)
                json_array_append_new(cur, json_null());
            cur = json_array_get(cur, idx);
        } else
            return NULL;
    }
    json_incref(cur);
    return cur;
}

/* ── set ────────────────────────────────────────────────────────────── */

int celix_json_pointer_set(json_t* doc, const celix_json_pointer_t* ptr, json_t* value) {
    if (!doc || !ptr || ptr->len == 0)
        return -1;

    json_t* cur = doc;
    for (size_t i = 0; i < ptr->len - 1; i++) {
        const char* tok = ptr->tokens[i];

        if (json_is_object(cur)) {
            json_t* child = json_object_get(cur, tok);
            if (!child) {
                /* Look ahead: if next token is numeric → array, else object */
                bool next_num = false;
                if (i + 1 < ptr->len) {
                    const char* next = ptr->tokens[i + 1];
                    next_num = true;
                    if (*next == '\0')
                        next_num = false;
                    for (const char* c = next; *c; c++)
                        if (!is_digit(*c)) {
                            next_num = false;
                            break;
                        }
                    if (next_num && next[0] == '0' && next[1] != '\0')
                        next_num = false;
                }
                child = next_num ? json_array() : json_object();
                json_object_set_new(cur, tok, child);
            }
            cur = child;
        } else if (json_is_array(cur)) {
            for (const char* c = tok; *c; c++)
                if (!is_digit(*c)) {
                    json_decref(value);
                    return -1;
                }
            size_t idx = (size_t)strtoul(tok, NULL, 10);
            while (json_array_size(cur) <= idx)
                json_array_append_new(cur, json_null());
            json_t* child = json_array_get(cur, idx);
            /* Replace null/primitive intermediates with containers */
            if (!child || (!json_is_object(child) && !json_is_array(child))) {
                bool next_num = false;
                if (i + 1 < ptr->len) {
                    const char* next = ptr->tokens[i + 1];
                    next_num = true;
                    if (*next == '\0')
                        next_num = false;
                    for (const char* c = next; *c; c++)
                        if (!is_digit(*c)) {
                            next_num = false;
                            break;
                        }
                    if (next_num && next[0] == '0' && next[1] != '\0')
                        next_num = false;
                }
                json_t* repl = next_num ? json_array() : json_object();
                json_array_set_new(cur, idx, repl);
                child = repl;
            }
            cur = child;
        } else {
            json_decref(value);
            return -1;
        }
    }

    const char* last = ptr->tokens[ptr->len - 1];
    if (json_is_object(cur)) {
        json_object_set_new(cur, last, value);
        return 0;
    }
    if (json_is_array(cur)) {
        if (strcmp(last, "-") == 0) {
            json_array_append_new(cur, value);
            return 0;
        }
        for (const char* c = last; *c; c++)
            if (!is_digit(*c)) {
                json_decref(value);
                return -1;
            }
        size_t idx = (size_t)strtoul(last, NULL, 10);
        while (json_array_size(cur) <= idx)
            json_array_append_new(cur, json_null());
        json_array_set_new(cur, idx, value);
        return 0;
    }
    json_decref(value);
    return -1;
}

int celix_json_pointer_set_new(json_t* doc, const celix_json_pointer_t* ptr, json_t* value) {
    if (value)
        json_incref(value);
    return celix_json_pointer_set(doc, ptr, value);
}

/* ── remove ─────────────────────────────────────────────────────────── */

int celix_json_pointer_remove(json_t* doc, const celix_json_pointer_t* ptr) {
    if (!doc || !ptr || ptr->len == 0)
        return -1;

    celix_json_pointer_t parent_ptr;
    memset(&parent_ptr, 0, sizeof(parent_ptr));
    for (size_t i = 0; i < ptr->len - 1; i++)
        celix_json_pointer_push(&parent_ptr, ptr->tokens[i]);

    json_t* parent = celix_json_pointer_get(doc, &parent_ptr);
    celix_json_pointer_clear(&parent_ptr);
    if (!parent)
        return -1;

    const char* last = ptr->tokens[ptr->len - 1];
    if (json_is_object(parent)) {
        if (!json_object_get(parent, last))
            return -1;
        json_object_del(parent, last);
        return 0;
    }
    if (json_is_array(parent)) {
        for (const char* c = last; *c; c++)
            if (!is_digit(*c))
                return -1;
        size_t idx = (size_t)strtoul(last, NULL, 10);
        if (idx >= json_array_size(parent))
            return -1;
        json_array_remove(parent, idx);
        return 0;
    }
    return -1;
}

/* ── Escape / unescape ──────────────────────────────────────────────── */

char* celix_json_pointer_escape(const char* token) {
    if (!token)
        return NULL;
    size_t len = 0;
    for (const char* c = token; *c; c++)
        len += (*c == '~' || *c == '/') ? 2 : 1;
    char* out = (char*)malloc(len + 1);
    if (!out)
        return NULL;
    char* w = out;
    for (const char* c = token; *c; c++) {
        if (*c == '~') {
            *w++ = '~';
            *w++ = '0';
        } else if (*c == '/') {
            *w++ = '~';
            *w++ = '1';
        } else
            *w++ = *c;
    }
    *w = '\0';
    return out;
}

char* celix_json_pointer_unescape(const char* token) {
    if (!token)
        return NULL;
    size_t len = strlen(token);
    char* out = (char*)malloc(len + 1);
    if (!out)
        return NULL;
    size_t di = 0;
    for (size_t si = 0; si < len; si++) {
        if (token[si] == '~' && si + 1 < len) {
            if (token[si + 1] == '0') {
                out[di++] = '~';
                si++;
            } else if (token[si + 1] == '1') {
                out[di++] = '/';
                si++;
            } else {
                out[di++] = '~';
            }
        } else
            out[di++] = token[si];
    }
    out[di] = '\0';
    return out;
}

/* ── Navigation ─────────────────────────────────────────────────────── */

celix_json_pointer_t* celix_json_pointer_parent(const celix_json_pointer_t* ptr, celix_json_pointer_t* out) {
    if (!ptr || ptr->len == 0)
        return NULL;
    celix_json_pointer_t* r = out ? out : (celix_json_pointer_t*)calloc(1, sizeof(*r));
    if (!r)
        return NULL;
    if (!out)
        celix_json_pointer_clear(r);
    if (ensure_cap(r, ptr->len - 1) != 0) {
        if (!out)
            free(r);
        return NULL;
    }
    for (size_t i = 0; i < ptr->len - 1; i++) {
        r->tokens[i] = strdup(ptr->tokens[i]);
        if (!r->tokens[i]) {
            celix_json_pointer_clear(r);
            if (!out)
                free(r);
            return NULL;
        }
        r->len++;
    }
    return r;
}

int celix_json_pointer_concat(celix_json_pointer_t* ptr, const celix_json_pointer_t* suffix) {
    if (!ptr || !suffix)
        return -1;
    for (size_t i = 0; i < suffix->len; i++)
        if (celix_json_pointer_push(ptr, suffix->tokens[i]) != 0)
            return -1;
    return 0;
}

/* ── Comparison ─────────────────────────────────────────────────────── */

int celix_json_pointer_equals(const celix_json_pointer_t* a, const celix_json_pointer_t* b) {
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    if (a->len != b->len)
        return false;
    for (size_t i = 0; i < a->len; i++) {
        if (strcmp(a->tokens[i], b->tokens[i]) != 0)
            return false;
    }
    return true;
}
