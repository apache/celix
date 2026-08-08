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
#include "celix_jansson_schema.h"
#include "celix_json_patch.h"
#include "celix_jansson_uri.h"
#include "celix_schema.h"
#include "celix_string_format_check.h"
#include "celix_util.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ════════════════════════════════════════════════════════════════════════
 * Error codes
 * ════════════════════════════════════════════════════════════════════════ */

const char* celix_jansson_schema_strerror(int err) {
    switch (err) {
    case CELIX_JANSSON_SCHEMA_OK:
        return "success";
    case CELIX_JANSSON_SCHEMA_ERROR_NOMEM:
        return "allocation failure";
    case CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA:
        return "schema must be boolean or object";
    case CELIX_JANSSON_SCHEMA_ERROR_SCHEMA_PARSE:
        return "JSON parse error";
    case CELIX_JANSSON_SCHEMA_ERROR_URI:
        return "malformed URI";
    case CELIX_JANSSON_SCHEMA_ERROR_REF_UNRESOLVED:
        return "unresolved $ref";
    case CELIX_JANSSON_SCHEMA_ERROR_LOADER:
        return "schema loader failed or absent";
    case CELIX_JANSSON_SCHEMA_ERROR_FORMAT_CHECKER:
        return "format checker required but not provided";
    case CELIX_JANSSON_SCHEMA_ERROR_CONTENT_CHECKER:
        return "content checker required but not provided";
    case CELIX_JANSSON_SCHEMA_ERROR_DUPLICATE_URI:
        return "duplicate URI";
    case CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN:
        return "invalid regex pattern";
    case CELIX_JANSSON_SCHEMA_ERROR_NO_ROOT_SCHEMA:
        return "no root schema set";
    case CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT:
        return "invalid argument";
    default:
        return "unknown error";
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * Object node helpers
 * ════════════════════════════════════════════════════════════════════════ */

/* The properties/dependencies maps are created lazily (only when the matching
 * schema keyword is present), so they can be NULL for a valid object node. */
static celix_jansson_schema_node_t* obj_node_get(celix_string_hash_map_t* map, const char* key) {
    return map ? (celix_jansson_schema_node_t*)celix_stringHashMap_get(map, key) : NULL;
}

/* Create a string hash map whose values are owning schema node refs; the
 * removed callback unrefs values when they leave the map. Returns NULL on OOM. */
static celix_string_hash_map_t* obj_node_map_create(void) {
    celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    opts.simpleRemovedCallback = (void (*)(void*))celix_jansson_schema_unref;
    return celix_stringHashMap_createWithOptions(&opts);
}

/* ════════════════════════════════════════════════════════════════════════
 * Path stack
 * ════════════════════════════════════════════════════════════════════════ */

void celix_jansson_path_init(celix_jansson_path_t* p) { memset(p, 0, sizeof(*p)); }

int celix_jansson_path_push(celix_jansson_path_t* p, const char* token) {
    if (p->len >= p->cap) {
        size_t nc = p->cap ? p->cap * 2 : 8;
        char** nt = (char**)realloc(p->tokens, nc * sizeof(char*));
        if (!nt)
            return -1;
        p->tokens = nt;
        p->cap = nc;
    }
    p->tokens[p->len] = strdup(token);
    if (!p->tokens[p->len])
        return -1;
    p->len++;
    free(p->cached);
    p->cached = NULL;
    return 0;
}

void celix_jansson_path_pop(celix_jansson_path_t* p) {
    if (p->len > 0) {
        p->len--;
        free(p->tokens[p->len]);
        p->tokens[p->len] = NULL;
    }
    free(p->cached);
    p->cached = NULL;
}

const char* celix_jansson_path_str(celix_jansson_path_t* p) {
    if (!p->cached) {
        celix_jansson_strbuf_t sb;
        celix_jansson_strbuf_init(&sb);
        for (size_t i = 0; i < p->len; i++) {
            celix_jansson_strbuf_appendc(&sb, '/');
            for (const char* c = p->tokens[i]; *c; c++) {
                if (*c == '~')
                    celix_jansson_strbuf_appends(&sb, "~0");
                else if (*c == '/')
                    celix_jansson_strbuf_appends(&sb, "~1");
                else
                    celix_jansson_strbuf_appendc(&sb, *c);
            }
        }
        p->cached = celix_jansson_strbuf_detach(&sb);
        if (!p->cached)
            p->cached = strdup("");
    }
    return p->cached;
}

void celix_jansson_path_free(celix_jansson_path_t* p) {
    for (size_t i = 0; i < p->len; i++)
        free(p->tokens[i]);
    free(p->tokens);
    free(p->cached);
    memset(p, 0, sizeof(*p));
}

/* ════════════════════════════════════════════════════════════════════════
 * Reference counting
 * ════════════════════════════════════════════════════════════════════════ */

celix_jansson_schema_node_t* celix_jansson_schema_ref(celix_jansson_schema_node_t* n) {
    if (n)
        __atomic_fetch_add(&n->refcount, 1, __ATOMIC_SEQ_CST);
    return n;
}

void celix_jansson_schema_unref(celix_jansson_schema_node_t* n) {
    if (!n)
        return;
    if (__atomic_fetch_sub(&n->refcount, 1, __ATOMIC_SEQ_CST) == 1) {
        if (n->vtable && n->vtable->destroy)
            n->vtable->destroy(n);
    }
}

/* ════════════════════════════════════════════════════════════════════════
 * Error sinks (user, first-error, collecting)
 * ════════════════════════════════════════════════════════════════════════ */

static void
emit_error_v(celix_jansson_validation_context_t* ctx, celix_jansson_path_t* path, const char* fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    const char* ps = path ? celix_jansson_path_str(path) : "";
    ctx->sink->emit(ctx->sink, ps, NULL, buf);
}

static void emit_error(celix_jansson_validation_context_t* ctx, celix_jansson_path_t* path, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    emit_error_v(ctx, path, fmt, ap);
    va_end(ap);
}

/* -- user sink -- */
typedef struct {
    celix_jansson_error_sink_t base;
    celix_jansson_schema_error_fn fn;
    void* ud;
    int count;
    bool abort_on_error;
    celix_jansson_validation_context_t* ctx; /* back-pointer for setting ctx->aborted */
} user_sink_t;
static void user_emit(celix_jansson_error_sink_t* s, const char* p, json_t* i, const char* m) {
    user_sink_t* us = (user_sink_t*)s;
    if (us->fn)
        us->fn(p, i, m, us->ud);
    us->count++;
    if (us->abort_on_error && us->ctx) {
        us->ctx->aborted = true;
    }
}
static void user_destroy(celix_jansson_error_sink_t* s) { free(s); }

/* -- first-error sink -- */
typedef struct {
    celix_jansson_error_sink_t base;
    bool got;
    char* ptr;
    json_t* inst;
    char* msg;
} first_sink_t;
static void first_emit(celix_jansson_error_sink_t* s, const char* p, json_t* i, const char* m) {
    first_sink_t* fs = (first_sink_t*)s;
    if (!fs->got) {
        fs->got = true;
        fs->ptr = strdup(p);
        fs->msg = strdup(m);
        fs->inst = i;
        json_incref(i);
    }
}
static void first_destroy(celix_jansson_error_sink_t* s) {
    first_sink_t* fs = (first_sink_t*)s;
    free(fs->ptr);
    json_decref(fs->inst);
    free(fs->msg);
    free(fs);
}

/* -- collecting sink -- */
typedef struct {
    celix_jansson_error_sink_t base;
    celix_jansson_error_list_t list;
} collecting_sink_t;
static void coll_emit(celix_jansson_error_sink_t* s, const char* p, json_t* i, const char* m) {
    collecting_sink_t* cs = (collecting_sink_t*)s;
    celix_jansson_error_list_add(&cs->list, p, i, m);
}
static void coll_destroy(celix_jansson_error_sink_t* s) {
    collecting_sink_t* cs = (collecting_sink_t*)s;
    celix_jansson_error_list_clear(&cs->list);
    free(cs);
}
static void coll_propagate(collecting_sink_t* cs, celix_jansson_error_sink_t* parent, const char* prefix) {
    for (size_t i = 0; i < cs->list.len; i++) {
        celix_jansson_error_entry_t* e = &cs->list.entries[i];
        celix_jansson_strbuf_t sb;
        celix_jansson_strbuf_init(&sb);
        if (prefix)
            celix_jansson_strbuf_appends(&sb, prefix);
        celix_jansson_strbuf_appends(&sb, e->message);
        char* full = celix_jansson_strbuf_detach(&sb);
        parent->emit(parent, e->ptr, e->instance, full ? full : e->message);
        free(full);
    }
}

static collecting_sink_t* coll_new(void) {
    collecting_sink_t* cs = (collecting_sink_t*)calloc(1, sizeof(*cs));
    if (!cs)
        return NULL;
    cs->base.emit = coll_emit;
    cs->base.destroy = coll_destroy;
    celix_jansson_error_list_init(&cs->list);
    return cs;
}

/* ════════════════════════════════════════════════════════════════════════
 * Helpers
 * ════════════════════════════════════════════════════════════════════════ */

/* Cross-type numeric equality (integer vs float only) */
static bool jss_json_equal(json_t* a, json_t* b) {
    if (json_equal(a, b))
        return true;
    /* Integer vs real: compare numeric values (e.g., 0 == 0.0) */
    if ((json_is_integer(a) && json_is_real(b)) || (json_is_real(a) && json_is_integer(b))) {
        double va = json_is_real(a) ? json_real_value(a) : (double)json_integer_value(a);
        double vb = json_is_real(b) ? json_real_value(b) : (double)json_integer_value(b);
        return va == vb;
    }
    return false;
}

int celix_jansson_type_index(json_t* value) {
    switch (json_typeof(value)) {
    case JSON_NULL:
        return 0;
    case JSON_OBJECT:
        return 1;
    case JSON_ARRAY:
        return 2;
    case JSON_STRING:
        return 3;
    case JSON_TRUE:
    case JSON_FALSE:
        return 4;
    case JSON_INTEGER:
        return 5;
    case JSON_REAL:
        return 6;
    //LCOV_EXCL_START: all legal jansson types are covered by the cases above
    default:
        return -1;
    //LCOV_EXCL_STOP
    }
}

/* UTF-8 code point count (count non-continuation bytes) */
static size_t utf8_length(const char* s, size_t len) {
    size_t n = 0;
    for (size_t i = 0; i < len; i++)
        if ((s[i] & 0xC0) != 0x80)
            n++;
    return n;
}

/* Floating-point multipleOf check with epsilon */
static bool violates_multiple(double x, double m) {
    if (m == 0.0)
        return false;
    double r = remainder(x, m);
    double mult = fabs(x / m);
    if (mult > 1.0)
        r /= mult;
    double eps = fabs(nextafter(x, 0.0) - x);
    return fabs(r) > fabs(eps);
}

/* ════════════════════════════════════════════════════════════════════════
 * Validation destructors
 * ════════════════════════════════════════════════════════════════════════ */

static void d_boolean(celix_jansson_schema_node_t* n) { free(n); }
static void d_null(celix_jansson_schema_node_t* n) { free(n); }
static void d_booltype(celix_jansson_schema_node_t* n) { free(n); }

static void d_required(celix_jansson_schema_node_t* n) {
    for (size_t i = 0; i < n->u.required.len; i++)
        free(n->u.required.names[i]);
    free(n->u.required.names);
    free(n);
}

static void d_ref(celix_jansson_schema_node_t* n) {
    free(n->u.ref.id);
    json_decref(n->default_value);
    free(n);
}

static void d_type(celix_jansson_schema_node_t* n) {
    for (int i = 0; i < 7; i++)
        celix_jansson_schema_unref(n->u.type_schema.type_slots[i]);
    for (size_t i = 0; i < n->u.type_schema.logic_len; i++)
        celix_jansson_schema_unref(n->u.type_schema.logic[i]);
    free(n->u.type_schema.logic);
    celix_jansson_schema_unref(n->u.type_schema.if_schema);
    celix_jansson_schema_unref(n->u.type_schema.then_schema);
    celix_jansson_schema_unref(n->u.type_schema.else_schema);
    json_decref(n->u.type_schema.enum_values);
    json_decref(n->u.type_schema.const_value);
    json_decref(n->default_value);
    free(n);
}

static void d_str(celix_jansson_schema_node_t* n) {
    if (n->u.string.has_pattern)
        regfree(&n->u.string.pattern);
    free(n->u.string.pattern_str);
    free(n->u.string.format);
    free(n->u.string.content_encoding);
    free(n->u.string.content_media_type);
    json_decref(n->default_value);
    free(n);
}

static void d_numeric(celix_jansson_schema_node_t* n) {
    json_decref(n->default_value);
    free(n);
}

static void d_object(celix_jansson_schema_node_t* n) {
    for (size_t i = 0; i < n->u.object.required_len; i++)
        free(n->u.object.required[i]);
    free(n->u.object.required);
    celix_stringHashMap_destroy(n->u.object.properties); /* NULL-safe; values freed via creation-time removed callback */
    for (size_t i = 0; i < n->u.object.pp_len; i++) {
        regfree(&n->u.object.pattern_properties[i].re);
        celix_jansson_schema_unref(n->u.object.pattern_properties[i].sch);
    }
    free(n->u.object.pattern_properties);
    celix_jansson_schema_unref(n->u.object.additional_properties);
    celix_stringHashMap_destroy(n->u.object.dependencies);
    celix_jansson_schema_unref(n->u.object.property_names);
    json_decref(n->default_value);
    free(n);
}

static void d_array(celix_jansson_schema_node_t* n) {
    celix_jansson_schema_unref(n->u.array.items_schema);
    for (size_t i = 0; i < n->u.array.items_len; i++)
        celix_jansson_schema_unref(n->u.array.items[i]);
    free(n->u.array.items);
    celix_jansson_schema_unref(n->u.array.additional_items);
    celix_jansson_schema_unref(n->u.array.contains);
    json_decref(n->default_value);
    free(n);
}

static void d_not(celix_jansson_schema_node_t* n) {
    celix_jansson_schema_unref(n->u.not_schema.sub);
    free(n);
}

static void d_comb(celix_jansson_schema_node_t* n) {
    for (size_t i = 0; i < n->u.combination.len; i++)
        celix_jansson_schema_unref(n->u.combination.items[i]);
    free(n->u.combination.items);
    free(n);
}

/* ════════════════════════════════════════════════════════════════════════
 * Per-kind validators
 * ════════════════════════════════════════════════════════════════════════ */

/* -- boolean -- */
static int v_boolean(const celix_jansson_schema_node_t* n,
                     json_t* inst,
                     celix_jansson_path_t* p,
                     celix_jansson_validation_context_t* ctx) {
    if (!n->u.boolean.value)
        emit_error(ctx, p, "instance invalid as per false-schema");
    return n->u.boolean.value ? 0 : 1;
}
static const json_t* dv_boolean(const celix_jansson_schema_node_t* n,
                                celix_jansson_path_t* p,
                                const json_t* inst,
                                celix_jansson_validation_context_t* ctx) {
    (void)p;
    (void)inst;
    (void)ctx;
    return n->default_value;
}

static const schema_vtable vt_boolean = {v_boolean, dv_boolean, d_boolean};

/* -- null -- */
static int v_null(const celix_jansson_schema_node_t* n,
                  json_t* inst,
                  celix_jansson_path_t* p,
                  celix_jansson_validation_context_t* ctx) {
    (void)n;
    (void)inst; /* unused once the assert compiles out under NDEBUG */
    (void)p;
    (void)ctx;
    assert(json_is_null(inst)); /* only dispatched from type_slots[0] (JSON_NULL) */
    return 0;
}
static const schema_vtable vt_null = {v_null, NULL, d_null};

/* -- boolean_type -- */
static int v_booltype(const celix_jansson_schema_node_t* n,
                      json_t* inst,
                      celix_jansson_path_t* p,
                      celix_jansson_validation_context_t* ctx) {
    (void)n;
    (void)inst; /* unused once the assert compiles out under NDEBUG */
    (void)p;
    (void)ctx;
    assert(json_is_boolean(inst)); /* only dispatched from type_slots[4] (JSON_TRUE/JSON_FALSE) */
    return 0;
}
static const schema_vtable vt_booltype = {v_booltype, NULL, d_booltype};

/* -- ref -- */
static int v_ref(const celix_jansson_schema_node_t* n,
                 json_t* inst,
                 celix_jansson_path_t* p,
                 celix_jansson_validation_context_t* ctx) {
    /* target_weak is wired up at compile time (placeholder nodes are resolved in
     * phase 2), so a root self-ref ("$ref": "#") never needs a runtime fallback. */
    celix_jansson_schema_node_t* t = n->u.ref.target_weak;
    if (!t) {
        emit_error(ctx, p, "unresolved or freed schema-reference");
        return 1;
    }
    /* Guard against infinite recursion through circular $ref */
    if (ctx->ref_depth > 20) {
        emit_error(ctx, p, "exceeded maximum $ref recursion depth");
        return 1;
    }
    ctx->ref_depth++;
    int rc = t->vtable->validate(t, inst, p, ctx);
    ctx->ref_depth--;
    return rc;
}
static const json_t* dv_ref(const celix_jansson_schema_node_t* n,
                            celix_jansson_path_t* p,
                            const json_t* inst,
                            celix_jansson_validation_context_t* ctx) {
    if (n->default_value)
        return n->default_value;
    celix_jansson_schema_node_t* t = n->u.ref.target_weak;
    if (t && t->vtable->default_value)
        return t->vtable->default_value(t, p, inst, ctx);
    return NULL;
}
static const schema_vtable vt_ref = {v_ref, dv_ref, d_ref};

/* -- required -- */
static int v_required(const celix_jansson_schema_node_t* n,
                      json_t* inst,
                      celix_jansson_path_t* p,
                      celix_jansson_validation_context_t* ctx) {
    assert(json_is_object(inst)); /* deps are only validated from v_object on an object instance */
    int errs = 0;
    for (size_t i = 0; i < n->u.required.len; i++) {
        json_t* v = json_object_get(inst, n->u.required.names[i]);
        if (!v) {
            celix_jansson_path_t pp;
            celix_jansson_path_init(&pp);
            for (size_t pi = 0; pi < p->len; pi++)
                celix_jansson_path_push(&pp, p->tokens[pi]);
            celix_jansson_path_push(&pp, n->u.required.names[i]);
            emit_error(ctx, &pp, "required property '%s' not found in object", n->u.required.names[i]);
            celix_jansson_path_free(&pp);
            errs++;
        }
    }
    return errs;
}
static const schema_vtable vt_required = {v_required, NULL, d_required};

/* -- string -- */
static int v_string(const celix_jansson_schema_node_t* n,
                    json_t* inst,
                    celix_jansson_path_t* p,
                    celix_jansson_validation_context_t* ctx) {
    assert(json_is_string(inst)); /* only dispatched from type_slots[3] (JSON_STRING) */
    const char* s = json_string_value(inst);
    size_t len = strlen(s);
    int errs = 0;

    if (n->u.string.has_min_len) {
        size_t cplen = utf8_length(s, len);
        if (cplen < n->u.string.min_len) {
            emit_error(ctx, p, "instance string is too short (min=%zu, got=%zu)", n->u.string.min_len, cplen);
            errs++;
        }
    }
    if (n->u.string.has_max_len) {
        size_t cplen = utf8_length(s, len);
        if (cplen > n->u.string.max_len) {
            emit_error(ctx, p, "instance string is too long (max=%zu, got=%zu)", n->u.string.max_len, cplen);
            errs++;
        }
    }
    if (n->u.string.has_content) {
        celix_jansson_schema_root_t* root = n->root;
        /* Reachable: the compile-time guard only checks contentEncoding, so a
         * contentMediaType-only schema compiles without a checker. Keep this
         * runtime check (covered by ContentMediaTypeWithoutCheckerAtValidate). */
        if (!root->content) {
            emit_error(
                ctx, p, "a content checker was not provided but a contentEncoding/contentMediaType keyword is present");
            errs++;
        } else {
            int rc = root->content(n->u.string.content_encoding,
                                   n->u.string.content_media_type ? n->u.string.content_media_type : "",
                                   inst,
                                   root->content_ud);
            if (rc != CELIX_JANSSON_SCHEMA_OK) {
                emit_error(ctx, p, "content validation failed");
                errs++;
            }
        }
    }
    if (n->u.string.has_pattern) {
        if (regexec(&n->u.string.pattern, s, 0, NULL, 0) != 0) {
            emit_error(ctx, p, "instance string does not match pattern '%s'", n->u.string.pattern_str);
            errs++;
        }
    }
    if (n->u.string.has_format) {
        celix_jansson_schema_root_t* root = n->root;
        assert(root->format != NULL); /* compile-time guard: no checker → CELIX_JANSSON_SCHEMA_ERROR_FORMAT_CHECKER */
        int rc = root->format(n->u.string.format, s, root->format_ud);
        if (rc != CELIX_JANSSON_SCHEMA_OK) {
            emit_error(ctx, p, "format-checking failed: %s", n->u.string.format);
            errs++;
        }
    }
    return errs;
}
static const schema_vtable vt_string = {v_string, NULL, d_str};

/* -- numeric (int/float share the same logic, just different bounds types) -- */
static int v_numeric_int(const celix_jansson_schema_node_t* n,
                         json_t* inst,
                         celix_jansson_path_t* p,
                         celix_jansson_validation_context_t* ctx) {
    assert(json_is_integer(inst)); /* only dispatched from type_slots[5] (JSON_INTEGER) */
    json_int_t v = json_integer_value(inst);
    int errs = 0;
    if (n->u.numeric.has_min) {
        if (n->u.numeric.exclusive_min ? (v <= n->u.numeric.bounds.i.min) : (v < n->u.numeric.bounds.i.min)) {
            emit_error(ctx, p, "instance is below minimum of %lld", (long long)n->u.numeric.bounds.i.min);
            errs++;
        }
    }
    if (n->u.numeric.has_max) {
        if (n->u.numeric.exclusive_max ? (v >= n->u.numeric.bounds.i.max) : (v > n->u.numeric.bounds.i.max)) {
            emit_error(ctx, p, "instance exceeds maximum of %lld", (long long)n->u.numeric.bounds.i.max);
            errs++;
        }
    }
    if (n->u.numeric.has_mult) {
        if (violates_multiple((double)v, n->u.numeric.multiple_of)) {
            emit_error(ctx, p, "instance is not a multiple of %g", n->u.numeric.multiple_of);
            errs++;
        }
    }
    return errs;
}

static int v_numeric_float(const celix_jansson_schema_node_t* n,
                           json_t* inst,
                           celix_jansson_path_t* p,
                           celix_jansson_validation_context_t* ctx) {
    /* only dispatched from type_slots[6] (JSON_REAL) or the aliased slot 5 for JSON_INTEGER */
    assert(json_is_real(inst) || json_is_integer(inst));
    double v = json_is_real(inst) ? json_real_value(inst) : (double)json_integer_value(inst);
    int errs = 0;
    if (n->u.numeric.has_min) {
        if (n->u.numeric.exclusive_min ? (v <= n->u.numeric.bounds.f.min) : (v < n->u.numeric.bounds.f.min)) {
            emit_error(ctx, p, "instance is below minimum of %.16g", n->u.numeric.bounds.f.min);
            errs++;
        }
    }
    if (n->u.numeric.has_max) {
        if (n->u.numeric.exclusive_max ? (v >= n->u.numeric.bounds.f.max) : (v > n->u.numeric.bounds.f.max)) {
            emit_error(ctx, p, "instance exceeds maximum of %.16g", n->u.numeric.bounds.f.max);
            errs++;
        }
    }
    if (n->u.numeric.has_mult) {
        if (violates_multiple(v, n->u.numeric.multiple_of)) {
            emit_error(ctx, p, "instance is not a multiple of %g", n->u.numeric.multiple_of);
            errs++;
        }
    }
    return errs;
}
static const schema_vtable vt_numeric_int = {v_numeric_int, NULL, d_numeric};
static const schema_vtable vt_numeric_float = {v_numeric_float, NULL, d_numeric};

/* -- object -- */
static void obj_validate_deps(const celix_jansson_schema_node_t* n,
                              json_t* inst,
                              celix_jansson_path_t* p,
                              celix_jansson_validation_context_t* ctx,
                              int* errs);

static int v_object(const celix_jansson_schema_node_t* n,
                    json_t* inst,
                    celix_jansson_path_t* p,
                    celix_jansson_validation_context_t* ctx) {
    assert(json_is_object(inst)); /* only dispatched from type_slots[1] (JSON_OBJECT) */
    int errs = 0;
    size_t sz = json_object_size(inst);

    if (n->u.object.has_min_p && sz < n->u.object.min_p) {
        emit_error(ctx, p, "instance has too few properties (%zu < %zu)", sz, n->u.object.min_p);
        errs++;
    }
    if (n->u.object.has_max_p && sz > n->u.object.max_p) {
        emit_error(ctx, p, "instance has too many properties (%zu > %zu)", sz, n->u.object.max_p);
        errs++;
    }

    /* required check */
    for (size_t i = 0; i < n->u.object.required_len; i++) {
        if (!json_object_get(inst, n->u.object.required[i])) {
            emit_error(ctx, p, "required property '%s' not found in object", n->u.object.required[i]);
            errs++;
            if (ctx->aborted) return errs;
        }
    }

    /* per-property validation */
    const char* key;
    json_t* val;
    json_object_foreach(inst, key, val) {
        if (ctx->aborted) break;

        /* propertyNames */
        if (n->u.object.property_names) {
            json_t* kname = json_string(key);
            celix_jansson_path_t kp;
            celix_jansson_path_init(&kp);
            for (size_t pi = 0; pi < p->len; pi++)
                celix_jansson_path_push(&kp, p->tokens[pi]);
            celix_jansson_path_push(&kp, key);
            errs += n->u.object.property_names->vtable->validate(n->u.object.property_names, kname, &kp, ctx);
            celix_jansson_path_free(&kp);
            json_decref(kname);
            if (ctx->aborted) break;
        }

        celix_jansson_path_t cp;
        celix_jansson_path_init(&cp);
        for (size_t pi = 0; pi < p->len; pi++)
            celix_jansson_path_push(&cp, p->tokens[pi]);
        celix_jansson_path_push(&cp, key);

        bool matched = false;

        /* properties */
        celix_jansson_schema_node_t* prop = obj_node_get(n->u.object.properties, key);
        if (prop) {
            matched = true;
            errs += prop->vtable->validate(prop, val, &cp, ctx);
        }

        /* patternProperties */
        for (size_t j = 0; j < n->u.object.pp_len; j++) {
            if (ctx->aborted) break;
            if (regexec(&n->u.object.pattern_properties[j].re, key, 0, NULL, 0) == 0) {
                matched = true;
                errs += n->u.object.pattern_properties[j].sch->vtable->validate(
                    n->u.object.pattern_properties[j].sch, val, &cp, ctx);
            }
        }

        /* additionalProperties */
        if (!matched && n->u.object.additional_properties) {
            first_sink_t* fs = (first_sink_t*)calloc(1, sizeof(*fs));
            fs->base.emit = first_emit;
            fs->base.destroy = first_destroy;
            celix_jansson_validation_context_t fctx = *ctx;
            fctx.sink = &fs->base;
            n->u.object.additional_properties->vtable->validate(n->u.object.additional_properties, val, &cp, &fctx);
            if (fs->got) {
                emit_error(ctx, &cp, "validation failed for additional property '%s': %s", key, fs->msg ? fs->msg : "");
                errs++;
                if (ctx->aborted) { fs->base.destroy(&fs->base); celix_jansson_path_free(&cp); break; }
            }
            fs->base.destroy(&fs->base);
        }
        celix_jansson_path_free(&cp);
    }

    /* default values for missing properties */
    {
        const char* base_path = celix_jansson_path_str(p);
        if (n->u.object.properties) {
            CELIX_STRING_HASH_MAP_ITERATE(n->u.object.properties, iter) {
                const char* pk = iter.key;
                celix_jansson_schema_node_t* ps = (celix_jansson_schema_node_t*)iter.value.ptrValue;
                if (!json_object_get(inst, pk)) {
                    const json_t* def = NULL;
                    if (ps->vtable && ps->vtable->default_value) {
                        celix_jansson_path_t dp;
                        celix_jansson_path_init(&dp);
                        for (size_t pi = 0; pi < p->len; pi++)
                            celix_jansson_path_push(&dp, p->tokens[pi]);
                        celix_jansson_path_push(&dp, pk);
                        def = ps->vtable->default_value(ps, &dp, inst, ctx);
                        celix_jansson_path_free(&dp);
                    }
                    if (!def)
                        def = ps->default_value;
                    if (def) {
                        char pat[1024];
                        snprintf(pat, sizeof(pat), "%s/%s", base_path, pk);
                        celix_json_patch_add(ctx->patch, pat, json_incref((json_t*)def));
                    }
                }
            }
        }
    }

    /* dependencies */
    obj_validate_deps(n, inst, p, ctx, &errs);

    return errs;
}

static void obj_validate_deps(const celix_jansson_schema_node_t* n,
                              json_t* inst,
                              celix_jansson_path_t* p,
                              celix_jansson_validation_context_t* ctx,
                              int* errs) {
    if (!n->u.object.dependencies || celix_stringHashMap_size(n->u.object.dependencies) == 0)
        return;

    const char* key;
    json_t* val;
    json_object_foreach(inst, key, val) {
        if (ctx->aborted) break;
        celix_jansson_schema_node_t* dep = obj_node_get(n->u.object.dependencies, key);
        if (dep) {
            celix_jansson_path_t cp;
            celix_jansson_path_init(&cp);
            for (size_t pi = 0; pi < p->len; pi++)
                celix_jansson_path_push(&cp, p->tokens[pi]);
            celix_jansson_path_push(&cp, key);
            *errs += dep->vtable->validate(dep, inst, &cp, ctx);
            celix_jansson_path_free(&cp);
        }
    }
}

//LCOV_EXCL_START
static const json_t* dv_object(const celix_jansson_schema_node_t* n,
                               celix_jansson_path_t* p,
                               const json_t* inst,
                               celix_jansson_validation_context_t* ctx) {
    (void)p;
    (void)inst;
    (void)ctx;
    return n->default_value;
}
//LCOV_EXCL_STOP
static const schema_vtable vt_object = {v_object, dv_object, d_object};

/* -- array -- */
static int v_array(const celix_jansson_schema_node_t* n,
                   json_t* inst,
                   celix_jansson_path_t* p,
                   celix_jansson_validation_context_t* ctx) {
    assert(json_is_array(inst)); /* only dispatched from type_slots[2] (JSON_ARRAY) */
    size_t sz = json_array_size(inst);
    int errs = 0;

    if (n->u.array.has_min_i && sz < n->u.array.min_items) {
        emit_error(ctx, p, "instance has too few items (%zu < %zu)", sz, n->u.array.min_items);
        errs++;
    }
    if (n->u.array.has_max_i && sz > n->u.array.max_items) {
        emit_error(ctx, p, "instance has too many items (%zu > %zu)", sz, n->u.array.max_items);
        errs++;
    }
    if (n->u.array.unique_items) {
        for (size_t i = 0; i < sz; i++) {
            for (size_t j = i + 1; j < sz; j++) {
                if (jss_json_equal(json_array_get(inst, i), json_array_get(inst, j))) {
                    emit_error(ctx, p, "items have to be unique for this array");
                    errs++;
                    goto unique_done;
                }
            }
        }
    unique_done:;
    }

    /* items */
    if (n->u.array.items_schema) {
        for (size_t i = 0; i < sz; i++) {
            if (ctx->aborted) break;
            char idx[32];
            snprintf(idx, sizeof(idx), "%zu", i);
            celix_jansson_path_t cp;
            celix_jansson_path_init(&cp);
            for (size_t pi = 0; pi < p->len; pi++)
                celix_jansson_path_push(&cp, p->tokens[pi]);
            celix_jansson_path_push(&cp, idx);
            errs +=
                n->u.array.items_schema->vtable->validate(n->u.array.items_schema, json_array_get(inst, i), &cp, ctx);
            celix_jansson_path_free(&cp);
        }
    } else if (n->u.array.items_len > 0) {
        /* tuple form */
        for (size_t i = 0; i < sz && i < n->u.array.items_len; i++) {
            if (ctx->aborted) break;
            char idx[32];
            snprintf(idx, sizeof(idx), "%zu", i);
            celix_jansson_path_t cp;
            celix_jansson_path_init(&cp);
            for (size_t pi = 0; pi < p->len; pi++)
                celix_jansson_path_push(&cp, p->tokens[pi]);
            celix_jansson_path_push(&cp, idx);
            errs += n->u.array.items[i]->vtable->validate(n->u.array.items[i], json_array_get(inst, i), &cp, ctx);
            celix_jansson_path_free(&cp);
        }
        if (n->u.array.additional_items && sz > n->u.array.items_len) {
            for (size_t i = n->u.array.items_len; i < sz; i++) {
                if (ctx->aborted) break;
                char idx[32];
                snprintf(idx, sizeof(idx), "%zu", i);
                celix_jansson_path_t cp;
                celix_jansson_path_init(&cp);
                for (size_t pi = 0; pi < p->len; pi++)
                    celix_jansson_path_push(&cp, p->tokens[pi]);
                celix_jansson_path_push(&cp, idx);
                errs += n->u.array.additional_items->vtable->validate(
                    n->u.array.additional_items, json_array_get(inst, i), &cp, ctx);
                celix_jansson_path_free(&cp);
            }
        }
    }

    /* contains */
    if (n->u.array.contains) {
        bool found = false;
        for (size_t i = 0; i < sz && !found; i++) {
            first_sink_t* fs = (first_sink_t*)calloc(1, sizeof(*fs));
            fs->base.emit = first_emit;
            fs->base.destroy = first_destroy;
            celix_jansson_validation_context_t fctx = *ctx;
            fctx.sink = &fs->base;
            char idx[32];
            snprintf(idx, sizeof(idx), "%zu", i);
            celix_jansson_path_t cp;
            celix_jansson_path_init(&cp);
            for (size_t pi = 0; pi < p->len; pi++)
                celix_jansson_path_push(&cp, p->tokens[pi]);
            celix_jansson_path_push(&cp, idx);
            int rc = n->u.array.contains->vtable->validate(n->u.array.contains, json_array_get(inst, i), &cp, &fctx);
            celix_jansson_path_free(&cp);
            if (rc == 0)
                found = true;
            fs->base.destroy(&fs->base);
        }
        if (!found) {
            emit_error(ctx, p, "no element satisfies the 'contains' schema");
            errs++;
        }
    }

    return errs;
}
static const schema_vtable vt_array = {v_array, NULL, d_array};

/* -- not -- */
static int v_not(const celix_jansson_schema_node_t* n,
                 json_t* inst,
                 celix_jansson_path_t* p,
                 celix_jansson_validation_context_t* ctx) {
    first_sink_t* fs = (first_sink_t*)calloc(1, sizeof(*fs));
    fs->base.emit = first_emit;
    fs->base.destroy = first_destroy;
    celix_jansson_validation_context_t fctx = *ctx;
    fctx.sink = &fs->base;
    int sub_errs = n->u.not_schema.sub->vtable->validate(n->u.not_schema.sub, inst, p, &fctx);
    fs->base.destroy(&fs->base);
    if (sub_errs == 0) {
        emit_error(ctx, p, "the subschema has succeeded, but it is required to not validate");
        return 1;
    }
    return 0;
}
static const schema_vtable vt_not = {v_not, NULL, d_not};

/* -- combination (allOf / anyOf / oneOf) -- */
static int v_comb(const celix_jansson_schema_node_t* n,
                  json_t* inst,
                  celix_jansson_path_t* p,
                  celix_jansson_validation_context_t* ctx) {
    size_t old_patch = json_array_size(ctx->patch);
    int count = 0;
    collecting_sink_t* master = coll_new();
    if (!master) {
        /* Fail closed: return the same "combination failed, error emitted"
         * convention as the other failing paths below (0 = passed) */
        emit_error(ctx, p, "out of memory while validating combination");
        return 1;
    }

    for (size_t i = 0; i < n->u.combination.len; i++) {
        collecting_sink_t* cs = coll_new();
        if (!cs) {
            emit_error(ctx, p, "out of memory while validating combination");
            /* Flush the errors collected by the earlier branches, then clean up */
            coll_propagate(master, ctx->sink, NULL);
            master->base.destroy(&master->base);
            celix_json_patch_truncate(ctx->patch, old_patch);
            return 1;
        }
        celix_jansson_validation_context_t cctx = *ctx;
        cctx.sink = &cs->base;

        size_t branch_patch = json_array_size(ctx->patch);
        int sub = n->u.combination.items[i]->vtable->validate(n->u.combination.items[i], inst, p, &cctx);

        if (sub == 0)
            count++;
        else
            celix_json_patch_truncate(ctx->patch, branch_patch);

        char prefix[128];
        const char* kname = (n->kind == CELIX_JANSSON_SCHEMA_KIND_ALL_OF)   ? "allOf"
                            : (n->kind == CELIX_JANSSON_SCHEMA_KIND_ANY_OF) ? "anyOf"
                                                                            : "oneOf";
        snprintf(prefix, sizeof(prefix), "[combination: %s / case#%zu] ", kname, i);
        coll_propagate(cs, &master->base, prefix);
        cs->base.destroy(&cs->base);
    }

    if (n->kind == CELIX_JANSSON_SCHEMA_KIND_ALL_OF) {
        if (count < (int)n->u.combination.len) {
            emit_error(ctx,
                       p,
                       "at least one subschema has failed, but all of them are required to validate - %zu failed",
                       n->u.combination.len - (size_t)count);
            coll_propagate(master, ctx->sink, NULL);
            master->base.destroy(&master->base);
            celix_json_patch_truncate(ctx->patch, old_patch);
            return 1;
        }
    } else if (n->kind == CELIX_JANSSON_SCHEMA_KIND_ANY_OF) {
        if (count == 0) {
            emit_error(ctx,
                       p,
                       "no subschema has succeeded, but one of them is required to validate. Type: anyOf, number of "
                       "failed subschemas: %zu",
                       n->u.combination.len);
            coll_propagate(master, ctx->sink, NULL);
            master->base.destroy(&master->base);
            celix_json_patch_truncate(ctx->patch, old_patch);
            return 1;
        }
    } else { /* ONE_OF */
        if (count == 0) {
            emit_error(ctx,
                       p,
                       "no subschema has succeeded, but one of them is required to validate. Type: oneOf, number of "
                       "failed subschemas: %zu",
                       n->u.combination.len);
            coll_propagate(master, ctx->sink, NULL);
            master->base.destroy(&master->base);
            celix_json_patch_truncate(ctx->patch, old_patch);
            return 1;
        }
        if (count > 1) {
            emit_error(
                ctx, p, "more than one subschema has succeeded, but exactly one of them is required to validate");
            master->base.destroy(&master->base);
            celix_json_patch_truncate(ctx->patch, old_patch);
            return 1;
        }
    }
    master->base.destroy(&master->base);
    return 0;
}
static const schema_vtable vt_comb = {v_comb, NULL, d_comb};

/* -- type_schema (dispatcher) -- */
static int v_type(const celix_jansson_schema_node_t* n,
                  json_t* inst,
                  celix_jansson_path_t* p,
                  celix_jansson_validation_context_t* ctx) {
    int slot = celix_jansson_type_index(inst);
    int errs = 0;

    /* Type check */
    celix_jansson_schema_node_t* typed = n->u.type_schema.type_slots[slot];
    if (!typed) {
        emit_error(ctx, p, "unexpected instance type");
        return 1;
    }
    errs += typed->vtable->validate(typed, inst, p, ctx);
    if (ctx->aborted) return errs;

    /* enum */
    if (n->u.type_schema.has_enum) {
        bool found = false;
        json_t* ev = n->u.type_schema.enum_values;
        for (size_t i = 0; i < json_array_size(ev); i++) {
            if (jss_json_equal(inst, json_array_get(ev, i))) {
                found = true;
                break;
            }
        }
        if (!found) {
            emit_error(ctx, p, "instance not found in required enum");
            errs++;
        }
    }

    /* const */
    if (n->u.type_schema.has_const) {
        if (!jss_json_equal(inst, n->u.type_schema.const_value)) {
            emit_error(ctx, p, "instance not const");
            errs++;
        }
    }

    /* logical combinators */
    for (size_t i = 0; i < n->u.type_schema.logic_len; i++) {
        if (ctx->aborted) break;
        errs += n->u.type_schema.logic[i]->vtable->validate(n->u.type_schema.logic[i], inst, p, ctx);
    }

    /* if/then/else */
    if (n->u.type_schema.if_schema) {
        first_sink_t* fs = (first_sink_t*)calloc(1, sizeof(*fs));
        fs->base.emit = first_emit;
        fs->base.destroy = first_destroy;
        celix_jansson_validation_context_t fctx = *ctx;
        fctx.sink = &fs->base;
        int if_errs = n->u.type_schema.if_schema->vtable->validate(n->u.type_schema.if_schema, inst, p, &fctx);
        fs->base.destroy(&fs->base);

        if (if_errs == 0) {
            if (n->u.type_schema.then_schema) {
                errs += n->u.type_schema.then_schema->vtable->validate(n->u.type_schema.then_schema, inst, p, ctx);
                if (ctx->aborted) return errs;
            }
        } else {
            if (n->u.type_schema.else_schema) {
                errs += n->u.type_schema.else_schema->vtable->validate(n->u.type_schema.else_schema, inst, p, ctx);
                if (ctx->aborted) return errs;
            }
        }
    }

    /* Root default: null instance gets default_value */
    if (json_is_null(inst) && n->default_value) {
        celix_json_patch_add(ctx->patch, celix_jansson_path_str(p), json_incref(n->default_value));
    }

    return errs;
}
static const json_t* dv_type(const celix_jansson_schema_node_t* n,
                             celix_jansson_path_t* p,
                             const json_t* inst,
                             celix_jansson_validation_context_t* ctx) {
    (void)p;
    (void)inst;
    (void)ctx;
    return n->default_value;
}
static const schema_vtable vt_type = {v_type, dv_type, d_type};

/* ════════════════════════════════════════════════════════════════════════
 * celix_jansson_schema_make — the compiler
 * ════════════════════════════════════════════════════════════════════════ */

/* Forward declarations for functions defined later in this file */
static celix_jansson_schema_node_t* resolve_document_fragment(
    celix_jansson_schema_root_t* root, const char* location, const char* fragment,
    int depth);

static int schema_make_internal_depth(json_t* sch,
                                      celix_jansson_schema_root_t* root,
                                      const celix_jansson_uri_t* base,
                                      celix_jansson_uri_t* eff_base_out,
                                      celix_jansson_schema_node_t** out,
                                      int depth);
static int schema_make_internal(json_t* sch, celix_jansson_schema_root_t* root,
                                const celix_jansson_uri_t* base,
                                celix_jansson_schema_node_t** out,
                                int depth) {
    return schema_make_internal_depth(sch, root, base, NULL, out, depth);
}

static int make_type_schema(json_t* sch, celix_jansson_schema_root_t* root,
                            const celix_jansson_uri_t* base,
                            celix_jansson_schema_node_t** out,
                            int depth) {
    celix_jansson_schema_node_t* n = (celix_jansson_schema_node_t*)calloc(1, sizeof(*n));
    if (!n)
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    n->vtable = &vt_type;
    n->kind = CELIX_JANSSON_SCHEMA_KIND_TYPE;
    n->root = root;
    n->refcount = 1;

    /* Parse "type" keyword */
    json_t* type_val = json_object_get(sch, "type");
    if (type_val) {
        if (json_is_string(type_val)) {
            const char* tname = json_string_value(type_val);
            int slot = -1;
            if (strcmp(tname, "null") == 0)
                slot = 0;
            else if (strcmp(tname, "object") == 0)
                slot = 1;
            else if (strcmp(tname, "array") == 0)
                slot = 2;
            else if (strcmp(tname, "string") == 0)
                slot = 3;
            else if (strcmp(tname, "boolean") == 0)
                slot = 4;
            else if (strcmp(tname, "integer") == 0)
                slot = 5;
            else if (strcmp(tname, "number") == 0)
                slot = 6;
            if (slot >= 0) {
                /* Create per-type validator */
                celix_jansson_schema_node_t* typed = (celix_jansson_schema_node_t*)calloc(1, sizeof(*n));
                if (!typed) {
                    free(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                typed->root = root;
                typed->refcount = 1;
                switch (slot) {
                case 0:
                    typed->vtable = &vt_null;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_NULL;
                    break;
                case 1:
                    typed->vtable = &vt_object;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_OBJECT;
                    break;
                case 2:
                    typed->vtable = &vt_array;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_ARRAY;
                    break;
                case 3:
                    typed->vtable = &vt_string;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_STRING;
                    break;
                case 4:
                    typed->vtable = &vt_booltype;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_BOOLEAN_TYPE;
                    break;
                case 5:
                    typed->vtable = &vt_numeric_int;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_NUMERIC_INT;
                    break;
                case 6:
                    typed->vtable = &vt_numeric_float;
                    typed->kind = CELIX_JANSSON_SCHEMA_KIND_NUMERIC_FLOAT;
                    break;
                }
                n->u.type_schema.type_slots[slot] = typed;
            }
        } else if (json_is_array(type_val)) {
            for (size_t i = 0; i < json_array_size(type_val); i++) {
                const char* tname = json_string_value(json_array_get(type_val, i));
                int slot = -1;
                if (!tname)
                    continue;
                if (strcmp(tname, "null") == 0)
                    slot = 0;
                else if (strcmp(tname, "object") == 0)
                    slot = 1;
                else if (strcmp(tname, "array") == 0)
                    slot = 2;
                else if (strcmp(tname, "string") == 0)
                    slot = 3;
                else if (strcmp(tname, "boolean") == 0)
                    slot = 4;
                else if (strcmp(tname, "integer") == 0)
                    slot = 5;
                else if (strcmp(tname, "number") == 0)
                    slot = 6;
                if (slot >= 0) {
                    celix_jansson_schema_node_t* typed = (celix_jansson_schema_node_t*)calloc(1, sizeof(*n));
                    if (!typed) {
                        /* Earlier iterations of the array form may already have filled
                         * type_slots → unref(n) (d_type unrefs each slot) not free(n) */
                        celix_jansson_schema_unref(n);
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                    }
                    typed->root = root;
                    typed->refcount = 1;
                    switch (slot) {
                    case 0:
                        typed->vtable = &vt_null;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_NULL;
                        break;
                    case 1:
                        typed->vtable = &vt_object;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_OBJECT;
                        break;
                    case 2:
                        typed->vtable = &vt_array;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_ARRAY;
                        break;
                    case 3:
                        typed->vtable = &vt_string;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_STRING;
                        break;
                    case 4:
                        typed->vtable = &vt_booltype;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_BOOLEAN_TYPE;
                        break;
                    case 5:
                        typed->vtable = &vt_numeric_int;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_NUMERIC_INT;
                        break;
                    case 6:
                        typed->vtable = &vt_numeric_float;
                        typed->kind = CELIX_JANSSON_SCHEMA_KIND_NUMERIC_FLOAT;
                        break;
                    }
                    n->u.type_schema.type_slots[slot] = typed;
                }
            }
        }
    } else {
        /* No type specified — all slots */
        for (int slot = 0; slot < 7; slot++) {
            celix_jansson_schema_node_t* typed = (celix_jansson_schema_node_t*)calloc(1, sizeof(*n));
            if (!typed) {
                /* Earlier iterations may already have filled type_slots */
                celix_jansson_schema_unref(n);
                return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
            }
            typed->root = root;
            typed->refcount = 1;
            static const schema_vtable* slots[] = {
                &vt_null, &vt_object, &vt_array, &vt_string, &vt_booltype, &vt_numeric_int, &vt_numeric_float};
            typed->vtable = slots[slot];
            static const enum celix_jansson_schema_kind_e kinds[] = {CELIX_JANSSON_SCHEMA_KIND_NULL,
                                                                     CELIX_JANSSON_SCHEMA_KIND_OBJECT,
                                                                     CELIX_JANSSON_SCHEMA_KIND_ARRAY,
                                                                     CELIX_JANSSON_SCHEMA_KIND_STRING,
                                                                     CELIX_JANSSON_SCHEMA_KIND_BOOLEAN_TYPE,
                                                                     CELIX_JANSSON_SCHEMA_KIND_NUMERIC_INT,
                                                                     CELIX_JANSSON_SCHEMA_KIND_NUMERIC_FLOAT};
            typed->kind = kinds[slot];
            n->u.type_schema.type_slots[slot] = typed;
        }
    }

    /* alias: number also validates integer */
    if (n->u.type_schema.type_slots[6] && !n->u.type_schema.type_slots[5])
        n->u.type_schema.type_slots[5] = celix_jansson_schema_ref(n->u.type_schema.type_slots[6]);

    /* Parse string constraints */
    {
        celix_jansson_schema_node_t* snode = n->u.type_schema.type_slots[3]; /* string slot */
        if (snode) {
            json_t* v;
            if ((v = json_object_get(sch, "minLength"))) {
                snode->u.string.has_min_len = true;
                snode->u.string.min_len = (size_t)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "maxLength"))) {
                snode->u.string.has_max_len = true;
                snode->u.string.max_len = (size_t)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "pattern"))) {
                const char* ps = json_string_value(v);
                snode->u.string.pattern_str = strdup(ps);
                snode->u.string.has_pattern = true;
                if (regcomp(&snode->u.string.pattern, ps, REG_EXTENDED) != 0) {
                    d_type(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_INVALID_PATTERN;
                }
            }
            if ((v = json_object_get(sch, "format"))) {
                snode->u.string.has_format = true;
                snode->u.string.format = strdup(json_string_value(v));
                if (!root->format) {
                    d_type(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_FORMAT_CHECKER;
                }
            }
            if ((v = json_object_get(sch, "contentEncoding"))) {
                snode->u.string.has_content = true;
                snode->u.string.content_encoding = strdup(json_string_value(v));
                if (!root->content) {
                    d_type(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_CONTENT_CHECKER;
                }
            }
            if ((v = json_object_get(sch, "contentMediaType"))) {
                snode->u.string.has_content = true;
                snode->u.string.content_media_type = strdup(json_string_value(v));
            }
        }
    }

    /* Parse numeric constraints (apply to both int and float slots) */
    {
        json_t* v;
        for (int ns = 5; ns <= 6; ns++) {
            celix_jansson_schema_node_t* nnum = n->u.type_schema.type_slots[ns];
            if (!nnum)
                continue;
            if ((v = json_object_get(sch, "minimum"))) {
                nnum->u.numeric.has_min = true;
                if (ns == 5)
                    nnum->u.numeric.bounds.i.min = json_integer_value(v);
                else
                    nnum->u.numeric.bounds.f.min = json_is_real(v) ? json_real_value(v) : (double)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "maximum"))) {
                nnum->u.numeric.has_max = true;
                if (ns == 5)
                    nnum->u.numeric.bounds.i.max = json_integer_value(v);
                else
                    nnum->u.numeric.bounds.f.max = json_is_real(v) ? json_real_value(v) : (double)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "exclusiveMinimum"))) {
                nnum->u.numeric.exclusive_min = true;
                nnum->u.numeric.has_min = true;
                if (ns == 5)
                    nnum->u.numeric.bounds.i.min = json_integer_value(v);
                else
                    nnum->u.numeric.bounds.f.min = json_is_real(v) ? json_real_value(v) : (double)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "exclusiveMaximum"))) {
                nnum->u.numeric.exclusive_max = true;
                nnum->u.numeric.has_max = true;
                if (ns == 5)
                    nnum->u.numeric.bounds.i.max = json_integer_value(v);
                else
                    nnum->u.numeric.bounds.f.max = json_is_real(v) ? json_real_value(v) : (double)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "multipleOf"))) {
                nnum->u.numeric.has_mult = true;
                nnum->u.numeric.multiple_of = json_is_real(v) ? json_real_value(v) : (double)json_integer_value(v);
            }
        }
    }

    /* Parse object constraints */
    {
        celix_jansson_schema_node_t* onode = n->u.type_schema.type_slots[1];
        if (onode) {
            json_t* v;
            if ((v = json_object_get(sch, "minProperties"))) {
                onode->u.object.has_min_p = true;
                onode->u.object.min_p = (size_t)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "maxProperties"))) {
                onode->u.object.has_max_p = true;
                onode->u.object.max_p = (size_t)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "required"))) {
                size_t rlen = json_array_size(v);
                onode->u.object.required = (char**)calloc(rlen, sizeof(char*));
                /* Check before setting required_len (d_object loops over required_len) */
                if (rlen > 0 && !onode->u.object.required) {
                    celix_jansson_schema_unref(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                onode->u.object.required_len = rlen;
                for (size_t i = 0; i < rlen; i++)
                    onode->u.object.required[i] = strdup(json_string_value(json_array_get(v, i)));
            }
            if ((v = json_object_get(sch, "properties"))) {
                onode->u.object.properties = obj_node_map_create();
                if (!onode->u.object.properties) {
                    celix_jansson_schema_unref(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                const char* k;
                json_t* pv;
                json_object_foreach(v, k, pv) {
                    celix_jansson_schema_node_t* ps = NULL;
                    int rc = schema_make_internal(pv, root, base, &ps, depth + 1);
                    if (rc != CELIX_JANSSON_SCHEMA_OK) {
                        /* Propagate the subschema compile error; d_object destroys the
                         * properties map and unrefs the subschemas already inserted */
                        celix_jansson_schema_unref(n);
                        return rc;
                    }
                    celix_stringHashMap_put(onode->u.object.properties, k, ps);
                }
            }
            if ((v = json_object_get(sch, "patternProperties"))) {
                size_t ppc = json_object_size(v);
                onode->u.object.pattern_properties = (typeof(onode->u.object.pattern_properties))calloc(
                    ppc, sizeof(*onode->u.object.pattern_properties));
                /* Check before setting pp_len (d_object loops over pp_len) */
                if (ppc > 0 && !onode->u.object.pattern_properties) {
                    celix_jansson_schema_unref(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                onode->u.object.pp_len = ppc;
                const char* pk;
                json_t* psch;
                size_t idx = 0;
                json_object_foreach(v, pk, psch) {
                    onode->u.object.pattern_properties[idx].sch = NULL;
                    regcomp(&onode->u.object.pattern_properties[idx].re, pk, REG_EXTENDED);
                    int rc = schema_make_internal(psch, root, base,
                                                  &onode->u.object.pattern_properties[idx].sch, depth + 1);
                    if (rc != CELIX_JANSSON_SCHEMA_OK) {
                        for (size_t k = 0; k <= idx; k++) {
                            regfree(&onode->u.object.pattern_properties[k].re);
                            celix_jansson_schema_unref(onode->u.object.pattern_properties[k].sch);
                        }
                        free(onode->u.object.pattern_properties);
                        onode->u.object.pattern_properties = NULL;
                        onode->u.object.pp_len = 0;
                        celix_jansson_schema_unref(n);
                        return rc;
                    }
                    idx++;
                }
            }
            if ((v = json_object_get(sch, "additionalProperties"))) {
                if (json_is_object(v) || json_is_boolean(v)) {
                    int rc = schema_make_internal(v, root, base, &onode->u.object.additional_properties, depth + 1);
                    if (rc != CELIX_JANSSON_SCHEMA_OK) {
                        celix_jansson_schema_unref(n);
                        return rc;
                    }
                }
            }
            if ((v = json_object_get(sch, "dependencies"))) {
                onode->u.object.dependencies = obj_node_map_create();
                if (!onode->u.object.dependencies) {
                    celix_jansson_schema_unref(n);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                const char* dk;
                json_t* dv;
                json_object_foreach(v, dk, dv) {
                    celix_jansson_schema_node_t* dep = NULL;
                    if (json_is_array(dv)) {
                        /* Array form → required list */
                        celix_jansson_schema_node_t* rn = (celix_jansson_schema_node_t*)calloc(1, sizeof(*rn));
                        if (!rn) {
                            celix_jansson_schema_unref(n);
                            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                        }
                        rn->vtable = &vt_required;
                        rn->kind = CELIX_JANSSON_SCHEMA_KIND_REQUIRED;
                        rn->root = root;
                        rn->refcount = 1;
                        size_t alen = json_array_size(dv);
                        rn->u.required.names = (char**)calloc(alen, sizeof(char*));
                        /* Check before setting len (d_required loops over len) */
                        if (alen > 0 && !rn->u.required.names) {
                            free(rn); /* rn owns nothing yet */
                            celix_jansson_schema_unref(n);
                            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                        }
                        rn->u.required.len = alen;
                        for (size_t ai = 0; ai < alen; ai++)
                            rn->u.required.names[ai] = strdup(json_string_value(json_array_get(dv, ai)));
                        dep = rn;
                    } else {
                        int rc = schema_make_internal(dv, root, base, &dep, depth + 1);
                        if (rc != CELIX_JANSSON_SCHEMA_OK) {
                            celix_jansson_schema_unref(n);
                            return rc;
                        }
                    }
                    if (dep)
                        celix_stringHashMap_put(onode->u.object.dependencies, dk, dep);
                }
            }
            if ((v = json_object_get(sch, "propertyNames"))) {
                int rc = schema_make_internal(v, root, base, &onode->u.object.property_names, depth + 1);
                if (rc != CELIX_JANSSON_SCHEMA_OK) {
                    celix_jansson_schema_unref(n);
                    return rc;
                }
            }
        }
    }

    /* Parse array constraints */
    {
        celix_jansson_schema_node_t* anode = n->u.type_schema.type_slots[2];
        if (anode) {
            json_t* v;
            if ((v = json_object_get(sch, "minItems"))) {
                anode->u.array.has_min_i = true;
                anode->u.array.min_items = (size_t)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "maxItems"))) {
                anode->u.array.has_max_i = true;
                anode->u.array.max_items = (size_t)json_integer_value(v);
            }
            if ((v = json_object_get(sch, "uniqueItems")))
                anode->u.array.unique_items = json_is_true(v);
            if ((v = json_object_get(sch, "items"))) {
                if (json_is_object(v) || json_is_boolean(v)) {
                    int rc = schema_make_internal(v, root, base, &anode->u.array.items_schema, depth + 1);
                    if (rc != CELIX_JANSSON_SCHEMA_OK) {
                        celix_jansson_schema_unref(n);
                        return rc;
                    }
                } else if (json_is_array(v)) {
                    size_t ilen = json_array_size(v);
                    anode->u.array.items =
                        (celix_jansson_schema_node_t**)calloc(ilen, sizeof(celix_jansson_schema_node_t*));
                    /* Check before setting items_len (d_array loops over items_len) */
                    if (ilen > 0 && !anode->u.array.items) {
                        celix_jansson_schema_unref(n);
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                    }
                    anode->u.array.items_len = ilen;
                    for (size_t i = 0; i < ilen; i++) {
                        int rc = schema_make_internal(json_array_get(v, i), root, base,
                                                      &anode->u.array.items[i], depth + 1);
                        if (rc != CELIX_JANSSON_SCHEMA_OK) {
                            for (size_t k = 0; k < i; k++)
                                celix_jansson_schema_unref(anode->u.array.items[k]);
                            free(anode->u.array.items);
                            anode->u.array.items = NULL;
                            anode->u.array.items_len = 0;
                            celix_jansson_schema_unref(n);
                            return rc;
                        }
                    }
                }
            }
            if ((v = json_object_get(sch, "additionalItems"))) {
                int rc = schema_make_internal(v, root, base, &anode->u.array.additional_items, depth + 1);
                if (rc != CELIX_JANSSON_SCHEMA_OK) {
                    celix_jansson_schema_unref(n);
                    return rc;
                }
            }
            if ((v = json_object_get(sch, "contains"))) {
                int rc = schema_make_internal(v, root, base, &anode->u.array.contains, depth + 1);
                if (rc != CELIX_JANSSON_SCHEMA_OK) {
                    celix_jansson_schema_unref(n);
                    return rc;
                }
            }
        }
    }

    /* enum */
    {
        json_t* v = json_object_get(sch, "enum");
        if (v) {
            n->u.type_schema.has_enum = true;
            n->u.type_schema.enum_values = json_deep_copy(v);
        }
    }

    /* const */
    {
        json_t* v = json_object_get(sch, "const");
        if (v) {
            n->u.type_schema.has_const = true;
            n->u.type_schema.const_value = json_deep_copy(v);
        }
    }

    /* logic combinators: not, allOf, anyOf, oneOf */
    {
        json_t* v;
        celix_jansson_vec_t logic;
        celix_jansson_vec_init(&logic);
        if ((v = json_object_get(sch, "not"))) {
            celix_jansson_schema_node_t* sub;
            int rc = schema_make_internal(v, root, base, &sub, depth + 1);
            if (rc != CELIX_JANSSON_SCHEMA_OK) {
                celix_jansson_schema_unref(n);
                celix_jansson_vec_free(&logic);
                return rc;
            }
            celix_jansson_schema_node_t* nn = (celix_jansson_schema_node_t*)calloc(1, sizeof(*nn));
            if (!nn) {
                celix_jansson_schema_unref(sub); /* sub was compiled successfully above */
                celix_jansson_schema_unref(n);
                celix_jansson_vec_free(&logic);
                return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
            }
            nn->vtable = &vt_not;
            nn->kind = CELIX_JANSSON_SCHEMA_KIND_NOT;
            nn->root = root;
            nn->refcount = 1;
            nn->u.not_schema.sub = sub;
            celix_jansson_vec_push(&logic, nn);
        }
        static const char* combos[] = {"allOf", "anyOf", "oneOf"};
        static const enum celix_jansson_schema_kind_e ckinds[] = {
            CELIX_JANSSON_SCHEMA_KIND_ALL_OF, CELIX_JANSSON_SCHEMA_KIND_ANY_OF, CELIX_JANSSON_SCHEMA_KIND_ONE_OF};
        for (int ci = 0; ci < 3; ci++) {
            if ((v = json_object_get(sch, combos[ci]))) {
                size_t clen = json_array_size(v);
                celix_jansson_schema_node_t* cn = (celix_jansson_schema_node_t*)calloc(1, sizeof(*cn));
                if (!cn) {
                    /* logic may already hold a not node or earlier combos */
                    for (size_t k = 0; k < logic.len; k++)
                        celix_jansson_schema_unref((celix_jansson_schema_node_t*)logic.items[k]);
                    celix_jansson_schema_unref(n);
                    celix_jansson_vec_free(&logic);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                cn->vtable = &vt_comb;
                cn->kind = ckinds[ci];
                cn->root = root;
                cn->refcount = 1;
                cn->u.combination.items =
                    (celix_jansson_schema_node_t**)calloc(clen, sizeof(celix_jansson_schema_node_t*));
                /* Check before setting len (d_comb loops over len) */
                if (clen > 0 && !cn->u.combination.items) {
                    free(cn); /* cn is not registered in logic yet */
                    for (size_t k = 0; k < logic.len; k++)
                        celix_jansson_schema_unref((celix_jansson_schema_node_t*)logic.items[k]);
                    celix_jansson_schema_unref(n);
                    celix_jansson_vec_free(&logic);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
                cn->u.combination.len = clen;
                for (size_t j = 0; j < clen; j++) {
                    int rc = schema_make_internal(json_array_get(v, j), root, base,
                                                  &cn->u.combination.items[j], depth + 1);
                    if (rc != CELIX_JANSSON_SCHEMA_OK) {
                        /* Propagate schema compile errors */
                        for (size_t k = 0; k < j; k++)
                            celix_jansson_schema_unref(cn->u.combination.items[k]);
                        free(cn->u.combination.items);
                        free(cn);
                        /* vec_free only frees the array, not the elements: unref the
                         * not/earlier-combo nodes pushed into logic so far */
                        for (size_t k = 0; k < logic.len; k++)
                            celix_jansson_schema_unref((celix_jansson_schema_node_t*)logic.items[k]);
                        celix_jansson_schema_unref(n);
                        celix_jansson_vec_free(&logic);
                        return rc;
                    }
                }
                celix_jansson_vec_push(&logic, cn);
            }
        }
        if (logic.len > 0) {
            n->u.type_schema.logic = (celix_jansson_schema_node_t**)logic.items;
            n->u.type_schema.logic_len = logic.len;
        }
    }

    /* if/then/else */
    {
        json_t* ifv = json_object_get(sch, "if");
        if (ifv) {
            int rc = schema_make_internal(ifv, root, base, &n->u.type_schema.if_schema, depth + 1);
            if (rc != CELIX_JANSSON_SCHEMA_OK) {
                celix_jansson_schema_unref(n);
                return rc;
            }
            json_t* thenv = json_object_get(sch, "then");
            if (thenv) {
                rc = schema_make_internal(thenv, root, base, &n->u.type_schema.then_schema, depth + 1);
                if (rc != CELIX_JANSSON_SCHEMA_OK) {
                    celix_jansson_schema_unref(n);
                    return rc;
                }
            }
            json_t* elsev = json_object_get(sch, "else");
            if (elsev) {
                rc = schema_make_internal(elsev, root, base, &n->u.type_schema.else_schema, depth + 1);
                if (rc != CELIX_JANSSON_SCHEMA_OK) {
                    celix_jansson_schema_unref(n);
                    return rc;
                }
            }
        }
    }

    /* default */
    {
        json_t* dv = json_object_get(sch, "default");
        if (dv)
            n->default_value = json_deep_copy(dv);
    }

    *out = n;
    return CELIX_JANSSON_SCHEMA_OK;
}

static int schema_make_internal_depth(json_t* sch,
                                      celix_jansson_schema_root_t* root,
                                      const celix_jansson_uri_t* base,
                                      celix_jansson_uri_t* eff_base_out,
                                      celix_jansson_schema_node_t** out,
                                      int depth) {
    //LCOV_EXCL_START: defensive — all call sites pass non-NULL arguments
    if (!sch || !root || !out)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    //LCOV_EXCL_STOP
    /* Guard against infinite recursion for self-referencing schemas */
    if (depth > 20)
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA;

    /* ── Parse $id to compute effective base URI ─────────────────────── */
    const celix_jansson_uri_t* effective_base = base;
    celix_jansson_uri_t my_base;
    memset(&my_base, 0, sizeof(my_base)); /* must zero-init for uri_derive → uri_clear */
    bool has_id = false;
    bool id_stored_in_out = false; /* true if eff_base_out holds the owned URI */
    json_t* refv = json_object_get(sch, "$ref"); /* peek early: $id alongside $ref is not a real $id */
    json_t* idv = json_object_get(sch, "$id");
    if (idv && json_is_string(idv) && !refv) {
        const celix_jansson_uri_t* derive_from = effective_base;
        celix_jansson_uri_t empty;
        if (!derive_from) {
            celix_jansson_uri_init(&empty, "");
            derive_from = &empty;
        }
        if (eff_base_out) {
            /* Derive directly into caller's buffer — avoids double-free from struct copy */
            if (celix_jansson_uri_derive(derive_from, json_string_value(idv), eff_base_out) == 0) {
                effective_base = eff_base_out;
                has_id = true;
                id_stored_in_out = true;
            }
        } else {
            if (celix_jansson_uri_derive(derive_from, json_string_value(idv), &my_base) == 0) {
                effective_base = &my_base;
                has_id = true;
            }
        }
        if (derive_from == &empty)
            celix_jansson_uri_clear(&empty);
    }

    if (json_is_boolean(sch))
    {
        celix_jansson_schema_node_t* n = (celix_jansson_schema_node_t*)calloc(1, sizeof(*n));
        if (!n)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        n->vtable = &vt_boolean;
        n->kind = CELIX_JANSSON_SCHEMA_KIND_BOOLEAN;
        n->root = root;
        n->refcount = 1;
        n->u.boolean.value = json_boolean_value(sch);
        *out = n;
        /* No $id registration: a boolean schema cannot carry $id (json_object_get
         * returns NULL for non-objects, so has_id is always false here) */
        return CELIX_JANSSON_SCHEMA_OK;
    }
    if (!json_is_object(sch))
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA;

    /* Process definitions first — compile and register using effective base location */
    json_t* defs = json_object_get(sch, "definitions");
    if (defs && json_is_object(defs)) {
        char* base_loc = effective_base ? celix_jansson_uri_location(effective_base) : strdup("");
        if (!base_loc) {
            /* Mirror the my_base cleanup of the other failing paths */
            if (has_id && !id_stored_in_out)
                celix_jansson_uri_clear(&my_base);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        celix_jansson_schema_file_t* sf = celix_jansson_schema_root_get_or_create_file(root, base_loc);
        free(base_loc);
        if (!sf) {
            /* Mirror the my_base cleanup of the other failing paths */
            if (has_id && !id_stored_in_out)
                celix_jansson_uri_clear(&my_base);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        const char* dk;
        json_t* dv;
        json_object_foreach(defs, dk, dv) {
            celix_jansson_schema_node_t* ds;
            int rc = schema_make_internal(dv, root, effective_base, &ds, depth + 1);
            if (rc != CELIX_JANSSON_SCHEMA_OK) {
                if (has_id && !id_stored_in_out)
                    celix_jansson_uri_clear(&my_base);
                return rc;
            }
            if (ds) {
                char frag[1024];
                snprintf(frag, sizeof(frag), "/definitions/%s", dk);
                celix_jansson_schema_node_t* existing =
                    (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->schemas, frag);
                if (existing) {
                    celix_jansson_schema_unref(ds);
                } else {
                    celix_stringHashMap_put(sf->schemas, frag, ds);
                }
            }
        }
    }

    /* Check for $ref */
    if (refv) {
        const char* ref_str = json_string_value(refv);
        if (!ref_str)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_SCHEMA;

        /* Resolve ref_str against the effective base URI */
        celix_jansson_uri_t ref_uri;
        memset(&ref_uri, 0, sizeof(ref_uri)); /* must zero-init for uri_derive → uri_clear */
        if (effective_base) {
            if (celix_jansson_uri_derive(effective_base, ref_str, &ref_uri) != 0)
                return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        } else {
            if (celix_jansson_uri_init(&ref_uri, ref_str) != 0)
                return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* init clears ref_uri on failure */
        }

        char* rloc = celix_jansson_uri_location(&ref_uri);
        char* rfra = celix_jansson_uri_fragment(&ref_uri);
        if (!rloc || !rfra) {
            free(rloc);
            free(rfra);
            celix_jansson_uri_clear(&ref_uri);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        celix_jansson_schema_file_t* sf = celix_jansson_schema_root_get_or_create_file(root, rloc);
        celix_jansson_schema_node_t* target = NULL;
        bool plain_self_ref = (strcmp(ref_str, "#") == 0 && rloc[0] == '\0' && rfra[0] == '\0');
        if (sf) {
            target = (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->schemas, rfra);
            /* Document fragment walk for internal refs and external docs with document loaded */
            if (!target && !plain_self_ref && (sf->document || (rloc[0] == '\0' && root->original_schema))) {
                target = resolve_document_fragment(root, rloc, rfra, depth + 1);
            }
            if (!target) {
                target = (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->unresolved, rfra);
            }
            if (!target) {
                target = (celix_jansson_schema_node_t*)calloc(1, sizeof(*target));
                if (target) {
                    target->vtable = &vt_ref;
                    target->kind = CELIX_JANSSON_SCHEMA_KIND_REF;
                    target->root = root;
                    target->refcount = 1;
                    char* uristr = celix_jansson_uri_to_string(&ref_uri);
                    if (!uristr) {
                        /* target is not yet in the unresolved map; unref releases it */
                        celix_jansson_schema_unref(target);
                        free(rloc);
                        free(rfra);
                        celix_jansson_uri_clear(&ref_uri);
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                    }
                    target->u.ref.id = uristr;
                    celix_stringHashMap_put(sf->unresolved, rfra, target);
                }
            }
        }

        if (!target && !plain_self_ref) {
            free(rloc); free(rfra); celix_jansson_uri_clear(&ref_uri);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }

        /* Create ref node — store derived URI as id */
        celix_jansson_schema_node_t* rn = (celix_jansson_schema_node_t*)calloc(1, sizeof(*rn));
        if (!rn) {
            free(rloc); free(rfra); celix_jansson_uri_clear(&ref_uri);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        rn->vtable = &vt_ref;
        rn->kind = CELIX_JANSSON_SCHEMA_KIND_REF;
        rn->root = root;
        rn->refcount = 1;
        rn->u.ref.id = celix_jansson_uri_to_string(&ref_uri);
        if (!rn->u.ref.id) {
            /* rn is not in any map and target_weak is still NULL here; unref releases it */
            celix_jansson_schema_unref(rn);
            free(rloc);
            free(rfra);
            celix_jansson_uri_clear(&ref_uri);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        if (target) {
            rn->u.ref.target_weak = target;
        }

        json_t* defv = json_object_get(sch, "default");
        if (defv)
            rn->default_value = json_deep_copy(defv);

        free(rloc); free(rfra);
        celix_jansson_uri_clear(&ref_uri);

        /* No $id registration: draft-7 treats $id alongside $ref as not a real
         * $id (JSON Reference: members other than $ref are ignored — see the
         * !refv guard above), so has_id is always false here */
        *out = rn;
        return CELIX_JANSSON_SCHEMA_OK;
    }

    int rc = make_type_schema(sch, root, effective_base, out, depth);
    if (rc == CELIX_JANSSON_SCHEMA_OK && has_id && *out) {
        int ir = celix_jansson_schema_root_insert(root, &my_base, *out);
        if (ir == CELIX_JANSSON_SCHEMA_ERROR_NOMEM) {
            /* The registry did not ref *out; unref it and clear it so the caller
             * does not take ownership on the error path */
            celix_jansson_schema_unref(*out);
            *out = NULL;
            if (!id_stored_in_out)
                celix_jansson_uri_clear(&my_base);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        /* DUPLICATE_URI is benign: with $id:"" the registration above already
         * happened and set_root_schema's root_insert must tolerate it as well */
    }
    if (has_id && !id_stored_in_out)
        celix_jansson_uri_clear(&my_base);
    return rc;
}

/* ════════════════════════════════════════════════════════════════════════
 * Document fragment resolution with $id tracking
 * ════════════════════════════════════════════════════════════════════════ */

/* Determine if a JSON pointer token points to a container whose values are
 * schema objects (definitions, properties, patternProperties, dependencies,
 * allOf, anyOf, oneOf, or the items-array tuple form). In such containers,
 * $id inside the member values is a real schema identifier. */
static bool token_members_are_schemas(const char* token, json_t* container) {
    static const char* schema_containers[] = {
        "definitions", "properties", "patternProperties", "dependencies",
        "allOf", "anyOf", "oneOf", NULL
    };
    for (const char** p = schema_containers; *p; p++)
        if (strcmp(token, *p) == 0)
            return true;
    /* items: tuple form = array of schemas, single form = one schema */
    if (strcmp(token, "items") == 0 && json_is_array(container))
        return true;
    return false;
}

/* Determine if a token points to a schema-position node (i.e., the value
 * at this key is itself a schema where $id would be meaningful). */
static bool token_is_schema_position(const char* token, json_t* child) {
    static const char* schema_positions[] = {
        "additionalProperties", "additionalItems", "contains",
        "propertyNames", "not", "if", "then", "else", NULL
    };
    for (const char** p = schema_positions; *p; p++)
        if (strcmp(token, *p) == 0)
            return true;
    /* "items" with a non-array value is a single schema */
    if (strcmp(token, "items") == 0 && !json_is_array(child))
        return true;
    return false;
}

/* Walk a JSON document following a JSON Pointer fragment, tracking $id
 * base-URI changes along the path. Compile the target subtree and register
 * it in the registry. */
static celix_jansson_schema_node_t* resolve_document_fragment(
    celix_jansson_schema_root_t* root, const char* location, const char* fragment,
    int depth)
{
    celix_jansson_schema_file_t* sf =
        (celix_jansson_schema_file_t*)celix_stringHashMap_get(root->files, location);
    assert(sf != NULL); /* all callers guard on sf first */

    json_t* doc = sf->document;
    if (!doc && location[0] == '\0')
        doc = root->original_schema;
    if (!doc)
        return NULL;

    /* Build the base URI for this walk */
    celix_jansson_uri_t cur_base;
    if (sf->base_uri) {
        if (celix_jansson_uri_init(&cur_base, sf->base_uri) != 0)
            return NULL; /* init leaves cur_base cleared */
    } else if (celix_jansson_uri_init(&cur_base, location[0] ? location : "") != 0) {
        return NULL;
    }

    /* Walk the fragment tokens */
    json_t* cur = doc;
    bool members_are_schemas = false;
    json_t* prev_container = NULL;

    if (fragment[0] == '/' && fragment[1] != '\0') {
        /* Tokenize: split by '/' and decode ~0/~1 */
        const char* s = fragment + 1; /* skip leading '/' */
        const char* tok_start = s;
        while (*tok_start) {
            const char* tok_end = tok_start;
            while (*tok_end && *tok_end != '/')
                tok_end++;

            /* Decode token */
            celix_jansson_strbuf_t tsb;
            celix_jansson_strbuf_init(&tsb);
            for (const char* c = tok_start; c < tok_end; c++) {
                if (*c == '~' && *(c+1) == '0') { celix_jansson_strbuf_appendc(&tsb, '~'); c++; }
                else if (*c == '~' && *(c+1) == '1') { celix_jansson_strbuf_appendc(&tsb, '/'); c++; }
                else celix_jansson_strbuf_appendc(&tsb, *c);
            }
            char* token = celix_jansson_strbuf_detach(&tsb);
            if (!token) {
                /* strbuf_detach also returns NULL for an empty buffer, so an
                 * empty token (double slash) is rejected here as well. */
                celix_jansson_uri_clear(&cur_base);
                return NULL;
            }

            prev_container = cur;

            if (!json_is_object(cur) && !json_is_array(cur)) {
                free(token);
                celix_jansson_uri_clear(&cur_base);
                return NULL;
            }

            json_t* child = NULL;
            if (json_is_array(cur)) {
                /* RFC 6901: array index token — must be a non-negative integer;
                 * an empty token never reaches this point (see detach above). */
                const char* t = token;
                for (const char* c = t; *c; c++) {
                    if (*c < '0' || *c > '9') {
                        free(token);
                        celix_jansson_uri_clear(&cur_base);
                        return NULL;
                    }
                }
                size_t idx = (size_t)strtoul(t, NULL, 10);
                child = (idx < json_array_size(cur)) ? json_array_get(cur, idx) : NULL;
            } else {
                child = json_object_get(cur, token);
            }

            /* If entering a schema-position node with $id, update base */
            if (child && json_is_object(child) &&
                (members_are_schemas || token_is_schema_position(token, child))) {
                json_t* cid = json_object_get(child, "$id");
                if (cid && json_is_string(cid)) {
                    celix_jansson_uri_t new_base;
                    memset(&new_base, 0, sizeof(new_base)); /* must zero-init for uri_derive → uri_clear */
                    if (celix_jansson_uri_derive(&cur_base, json_string_value(cid), &new_base) == 0) {
                        celix_jansson_uri_clear(&cur_base);
                        cur_base = new_base;
                    }
                }
            }

            members_are_schemas = token_members_are_schemas(token, child ? child : prev_container);

            if (!child) {
                free(token);
                celix_jansson_uri_clear(&cur_base);
                return NULL;
            }
            cur = child;
            free(token);

            tok_start = (*tok_end == '/') ? tok_end + 1 : tok_end;
        }
    }

    /* Compile the reached subtree with the tracked base */
    celix_jansson_schema_node_t* sch = NULL;
    int rc = schema_make_internal_depth(cur, root, &cur_base, NULL, &sch, depth + 1);
    celix_jansson_uri_clear(&cur_base);
    if (rc != CELIX_JANSSON_SCHEMA_OK || !sch)
        return NULL;

    /* Register under the full URI so waiting refs get resolved */
    celix_jansson_uri_t full_uri;
    if (celix_jansson_uri_init(&full_uri, location) != 0) {
        /* init leaves full_uri cleared; sch is owned by this frame */
        celix_jansson_schema_unref(sch);
        return NULL;
    }
    /* Re-attach fragment to the URI */
    if (fragment[0] == '/') {
        celix_json_pointer_t ptr;
        memset(&ptr, 0, sizeof(ptr));
        if (celix_json_pointer_init(&ptr, fragment) == 0) {
            bool push_ok = true;
            for (size_t i = 0; i < ptr.len; i++) {
                if (celix_json_pointer_push(&full_uri.pointer, ptr.tokens[i]) != 0) {
                    push_ok = false;
                    break;
                }
            }
            celix_json_pointer_clear(&ptr);
            if (!push_ok) {
                /* do not register under a partial fragment — it would wrongly
                 * satisfy a shorter-fragment waiting ref */
                celix_jansson_uri_clear(&full_uri);
                celix_jansson_schema_unref(sch);
                return NULL;
            }
        }
    }
    int ir = celix_jansson_schema_root_insert(root, &full_uri, sch);
    celix_jansson_uri_clear(&full_uri);
    //LCOV_EXCL_START: reachable only on OOM inside root_insert's location/fragment
    //allocation (the file cache-hit reasoning no longer applies); no current test drives this
    if (ir == CELIX_JANSSON_SCHEMA_ERROR_NOMEM) {
        /* The registry did not ref sch (NOMEM precedes the ref); unref it and
         * return NULL instead of a dangling pointer */
        celix_jansson_schema_unref(sch);
        return NULL;
    }
    //LCOV_EXCL_STOP
    /* DUPLICATE_URI is benign: the fragment may already be registered */
    celix_jansson_schema_unref(sch); /* registry holds the owning ref now */

    return sch;
}

/* ── Helper: count total unresolved refs across all files ───────────────── */
static size_t total_unresolved(const celix_jansson_schema_root_t* root) {
    size_t count = 0;
    CELIX_STRING_HASH_MAP_ITERATE(root->files, iter) {
        celix_jansson_schema_file_t* sf = (celix_jansson_schema_file_t*)iter.value.ptrValue;
        count += celix_stringHashMap_size(sf->unresolved);
    }
    return count;
}

/* ── Phase 2: compile external document ─────────────────────────────────── */
static int compile_external_document(celix_jansson_schema_root_t* root, const char* location) {
    if (!root->loader)
        return CELIX_JANSSON_SCHEMA_ERROR_LOADER;

    json_t* doc = NULL;
    int rc = root->loader(location, &doc, root->loader_ud);
    if (rc != CELIX_JANSSON_SCHEMA_OK || !doc) {
        json_decref(doc);
        return rc ? rc : CELIX_JANSSON_SCHEMA_ERROR_LOADER;
    }

    celix_jansson_uri_t retr;
    if (celix_jansson_uri_init(&retr, location) != 0) {
        json_decref(doc); /* doc has not been transferred to sf->document */
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* init leaves retr cleared */
    }
    celix_jansson_schema_node_t* sch = NULL;
    rc = schema_make_internal_depth(doc, root, &retr, NULL, &sch, 0);
    if (rc != CELIX_JANSSON_SCHEMA_OK || !sch) {
        json_decref(doc);
        celix_jansson_uri_clear(&retr);
        return rc ? rc : CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }

    /* Register root at retrieval URI — auto-resolves waiting fragment="" placeholder */
    int ir = celix_jansson_schema_root_insert(root, &retr, sch);
    //LCOV_EXCL_START: reachable only on OOM inside root_insert's location/fragment
    //allocation (the file cache-hit reasoning no longer applies); no current test drives this
    if (ir == CELIX_JANSSON_SCHEMA_ERROR_NOMEM) {
        /* The registry did not ref sch; propagate the OOM instead of returning a
         * fake OK that would leave the placeholder unresolved forever */
        json_decref(doc); /* doc has not been transferred to sf->document */
        celix_jansson_uri_clear(&retr);
        celix_jansson_schema_unref(sch);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    //LCOV_EXCL_STOP
    /* DUPLICATE_URI is benign: a document whose own $id equals the retrieval URI
     * was already registered during make */

    /* Retain document for fragment resolution */
    celix_jansson_schema_file_t* sf = celix_jansson_schema_root_get_or_create_file(root, location);
    if (sf) {
        json_decref(sf->document);
        sf->document = doc;  /* ownership transferred */
        /* strdup-then-swap: on OOM keep the old (content-identical) base_uri */
        char* nb = strdup(location);
        if (nb) {
            free(sf->base_uri);
            sf->base_uri = nb;
        }
    //LCOV_EXCL_START: unreachable — callers guarantee the file already exists
    } else {
        json_decref(doc);
    }
    //LCOV_EXCL_STOP

    celix_jansson_uri_clear(&retr);
    celix_jansson_schema_unref(sch);
    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── Phase 2: try to resolve a single placeholder ──────────────────────── */
static bool resolve_placeholder(celix_jansson_schema_root_t* root,
                                const char* location, const char* fragment,
                                celix_jansson_schema_node_t* ref_node) {
    /* Use the location+fragment to look up in schemas */
    celix_jansson_schema_file_t* sf =
        (celix_jansson_schema_file_t*)celix_stringHashMap_get(root->files, location);
    assert(sf != NULL); /* sole caller resolve_external_refs Phase B already guarantees sf */

    /* Already resolved? */
    celix_jansson_schema_node_t* existing =
        (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->schemas, fragment);
    if (existing) {
        /* ref_node is the REF-kind placeholder the caller fetched from sf->unresolved
         * (same table and key this function was called with), so wire it directly */
        ref_node->u.ref.target_weak = existing;
        /* the map's removed callback unrefs the value, so ref first: the ref
         * that survives the remove is handed over to sf->retained below */
        celix_jansson_schema_ref(ref_node);
        celix_stringHashMap_remove(sf->unresolved, fragment);
        celix_jansson_vec_push(&sf->retained, ref_node);
        return true;
    }

    /* Try document fragment walk */
    if (sf->document || (location[0] == '\0' && root->original_schema)) {
        celix_jansson_schema_node_t* resolved = resolve_document_fragment(root, location, fragment, 0);
        if (resolved)
            return true;
    }

    /* Try loading the document if not yet loaded */
    if (sf->document == NULL && location[0] != '\0' && root->loader) {
        int rc = compile_external_document(root, location);
        if (rc == CELIX_JANSSON_SCHEMA_OK) {
            /* Re-check schemas after loading */
            celix_jansson_schema_node_t* reloaded =
                (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->schemas, fragment);
            if (reloaded)
                return true;
            /* Try fragment walk again */
            return resolve_document_fragment(root, location, fragment, 0) != NULL;
        }
    }

    return false;
}

/* ── Phase 2: resolve all external refs ────────────────────────────────── */
static int resolve_external_refs(celix_jansson_schema_root_t* root) {
    int max_iter = 20;
    while (total_unresolved(root) > 0 && max_iter-- > 0) {
        bool progress = false;

        /* Phase A: load documents for locations with pending refs */
        {
            /* Snapshot file keys — loading adds new entries */
            celix_jansson_vec_t locs;
            celix_jansson_vec_init(&locs);
            CELIX_STRING_HASH_MAP_ITERATE(root->files, iter) {
                char* lk = strdup(iter.key);
                if (!lk || celix_jansson_vec_push(&locs, lk) != 0) {
                    free(lk); /* vec_push failure leaves the vec unchanged */
                    for (size_t j = 0; j < locs.len; j++)
                        free(locs.items[j]);
                    celix_jansson_vec_free(&locs);
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
                }
            }

            for (size_t i = 0; i < locs.len; i++) {
                const char* loc = (const char*)locs.items[i];
                if (loc[0] == '\0') { free(locs.items[i]); continue; }

                celix_jansson_schema_file_t* sf =
                    (celix_jansson_schema_file_t*)celix_stringHashMap_get(root->files, loc);
                if (sf && celix_stringHashMap_size(sf->unresolved) > 0 && sf->document == NULL) {
                    int rc = compile_external_document(root, loc);
                    if (rc != CELIX_JANSSON_SCHEMA_OK) {
                        for (size_t j = i; j < locs.len; j++)
                            free(locs.items[j]);
                        celix_jansson_vec_free(&locs);
                        return rc;
                    }
                    progress = true;
                }
                free(locs.items[i]);
            }
            celix_jansson_vec_free(&locs);
        }

        /* Phase B: resolve placeholders */
        {
            /* Snapshot (location, fragment) pairs */
            struct pf_pair { char* loc; char* frag; };
            celix_jansson_vec_t pairs;
            celix_jansson_vec_init(&pairs);
            CELIX_STRING_HASH_MAP_ITERATE(root->files, fiter) {
                celix_jansson_schema_file_t* sf = (celix_jansson_schema_file_t*)fiter.value.ptrValue;
                if (!sf || celix_stringHashMap_size(sf->unresolved) == 0)
                    continue;
                CELIX_STRING_HASH_MAP_ITERATE(sf->unresolved, uiter) {
                    struct pf_pair* p = (struct pf_pair*)malloc(sizeof(*p));
                    if (!p)
                        goto pairs_cleanup;
                    p->loc = strdup(fiter.key);
                    p->frag = strdup(uiter.key);
                    if (!p->loc || !p->frag || celix_jansson_vec_push(&pairs, p) != 0) {
                        free(p->loc);
                        free(p->frag);
                        free(p);
                        goto pairs_cleanup;
                    }
                }
            }

            for (size_t i = 0; i < pairs.len; i++) {
                struct pf_pair* p = (struct pf_pair*)pairs.items[i];
                celix_jansson_schema_file_t* sf =
                    (celix_jansson_schema_file_t*)celix_stringHashMap_get(root->files, p->loc);
                if (sf) {
                    celix_jansson_schema_node_t* ref_node =
                        (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->unresolved, p->frag);
                    if (ref_node)
                        progress |= resolve_placeholder(root, p->loc, p->frag, ref_node);
                }
            }
        pairs_cleanup:
            for (size_t i = 0; i < pairs.len; i++) {
                struct pf_pair* p = (struct pf_pair*)pairs.items[i];
                free(p->loc);
                free(p->frag);
                free(p);
            }
            celix_jansson_vec_free(&pairs);
        }

        if (!progress)
            break;
    }

    return total_unresolved(root) > 0
        ? (root->loader ? CELIX_JANSSON_SCHEMA_ERROR_REF_UNRESOLVED
                        : CELIX_JANSSON_SCHEMA_ERROR_LOADER)
        : CELIX_JANSSON_SCHEMA_OK;
}

/* ════════════════════════════════════════════════════════════════════════
 * Registry
 * ════════════════════════════════════════════════════════════════════════ */

static void celix_jansson_schema_file_free(void* value);

celix_jansson_schema_file_t* celix_jansson_schema_root_get_or_create_file(celix_jansson_schema_root_t* root,
                                                                          const char* location) {
    celix_jansson_schema_file_t* sf =
        (celix_jansson_schema_file_t*)celix_stringHashMap_get(root->files, location);
    if (sf)
        return sf;
    sf = (celix_jansson_schema_file_t*)calloc(1, sizeof(*sf));
    if (!sf)
        return NULL;
    sf->schemas = obj_node_map_create();
    sf->unresolved = obj_node_map_create();
    if (!sf->schemas || !sf->unresolved) {
        celix_stringHashMap_destroy(sf->schemas);
        celix_stringHashMap_destroy(sf->unresolved);
        free(sf);
        return NULL;
    }
    celix_jansson_vec_init(&sf->retained);
    celix_stringHashMap_put(root->files, location, sf);
    return sf;
}

int celix_jansson_schema_root_insert(celix_jansson_schema_root_t* root,
                                     const celix_jansson_uri_t* uri,
                                     celix_jansson_schema_node_t* sch) {
    char* loc = celix_jansson_uri_location(uri);
    const char* frag = celix_jansson_uri_fragment(uri);
    if (!loc || !frag) {
        free(loc);
        free((char*)frag);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    celix_jansson_schema_file_t* sf = celix_jansson_schema_root_get_or_create_file(root, loc);
    if (!sf) {
        free(loc);
        free((char*)frag);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    if (celix_stringHashMap_get(sf->schemas, frag)) {
        free(loc);
        free((char*)frag);
        return CELIX_JANSSON_SCHEMA_ERROR_DUPLICATE_URI;
    }
    celix_jansson_schema_ref(sch);
    celix_stringHashMap_put(sf->schemas, frag, sch);

    /* Resolve waiting refs */
    celix_jansson_schema_node_t* waiting =
        (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->unresolved, frag);
    if (waiting && waiting->kind == CELIX_JANSSON_SCHEMA_KIND_REF) {
        waiting->u.ref.target_weak = sch;
        /* the map's removed callback unrefs the value, so ref first: the ref
         * that survives the remove is handed over to sf->retained below */
        celix_jansson_schema_ref(waiting);
        celix_stringHashMap_remove(sf->unresolved, frag);
        celix_jansson_vec_push(&sf->retained, waiting);
    }
    free(loc);
    free((char*)frag);
    return CELIX_JANSSON_SCHEMA_OK;
}

static void celix_jansson_schema_file_free(void* value) {
    celix_jansson_schema_file_t* sf = (celix_jansson_schema_file_t*)value;
    assert(sf != NULL);
    celix_stringHashMap_destroy(sf->schemas); /* values freed via creation-time removed callback */
    celix_stringHashMap_destroy(sf->unresolved);
    for (size_t i = 0; i < sf->retained.len; i++)
        celix_jansson_schema_unref((celix_jansson_schema_node_t*)sf->retained.items[i]);
    celix_jansson_vec_free(&sf->retained);
    json_decref(sf->document);
    free(sf->base_uri);
    free(sf);
}

void celix_jansson_schema_root_destroy(celix_jansson_schema_root_t* root) {
    if (!root)
        return;
    celix_jansson_schema_unref(root->root);
    json_decref(root->original_schema);
    celix_stringHashMap_destroy(root->files); /* values freed via creation-time removed callback */
    free(root);
}

int celix_jansson_schema_root_validate(celix_jansson_schema_root_t* root,
                                       const char* initial_uri,
                                       json_t* instance,
                                       celix_jansson_validation_context_t* ctx) {
    if (!root || !ctx)
        return -1;

    celix_jansson_schema_node_t* sch = NULL;

    /* Resolve initial_uri to a specific subschema, if provided */
    if (initial_uri && initial_uri[0] != '\0' && strcmp(initial_uri, "#") != 0) {
        celix_jansson_uri_t uri;
        if (celix_jansson_uri_init(&uri, initial_uri) == 0) {
            char* loc = celix_jansson_uri_location(&uri);
            const char* frag = celix_jansson_uri_fragment(&uri);

            if (loc) {
                celix_jansson_schema_file_t* sf =
                    (celix_jansson_schema_file_t*)celix_stringHashMap_get(root->files, loc);
                if (sf) {
                    if (frag && frag[0] != '\0') {
                        sch = (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->schemas, frag);
                        if (!sch) {
                            /* Attempt on-demand compilation from the original document */
                            sch = resolve_document_fragment(root, loc, frag, 0);
                        }
                    } else {
                        /* Empty fragment — resolve the root of this file */
                        sch = (celix_jansson_schema_node_t*)celix_stringHashMap_get(sf->schemas, "");
                    }
                }
            }
            free(loc);
            free((char*)frag);
            celix_jansson_uri_clear(&uri);
        }
    }

    /* Fallback: use root schema */
    if (!sch)
        sch = root->root;

    if (!sch) {
        ctx->sink->emit(ctx->sink, "", instance, "no root schema set");
        return 1;
    }

    celix_jansson_path_t path;
    celix_jansson_path_init(&path);
    int errs = sch->vtable->validate(sch, instance, &path, ctx);
    celix_jansson_path_free(&path);
    return errs;
}

/* ════════════════════════════════════════════════════════════════════════
 * Error list
 * ════════════════════════════════════════════════════════════════════════ */

void celix_jansson_error_list_init(celix_jansson_error_list_t* el) { memset(el, 0, sizeof(*el)); }
void celix_jansson_error_list_add(celix_jansson_error_list_t* el, const char* ptr, json_t* inst, const char* msg) {
    if (el->len >= el->cap) {
        size_t nc = el->cap ? el->cap * 2 : 8;
        celix_jansson_error_entry_t* ne = (celix_jansson_error_entry_t*)realloc(el->entries, nc * sizeof(*ne));
        if (!ne)
            return;
        el->entries = ne;
        el->cap = nc;
    }
    celix_jansson_error_entry_t* e = &el->entries[el->len++];
    e->ptr = strdup(ptr);
    e->message = strdup(msg);
    e->instance = inst;
    json_incref(inst);
}
void celix_jansson_error_list_clear(celix_jansson_error_list_t* el) {
    for (size_t i = 0; i < el->len; i++) {
        free(el->entries[i].ptr);
        free(el->entries[i].message);
        json_decref(el->entries[i].instance);
    }
    free(el->entries);
    memset(el, 0, sizeof(*el));
}

/* ════════════════════════════════════════════════════════════════════════
 * Public API
 * ════════════════════════════════════════════════════════════════════════ */

struct celix_jansson_schema_validator_t {
    celix_jansson_schema_root_t* root;
    bool abort_on_error;
};

celix_jansson_schema_validator_t* celix_jansson_schema_validator_create(celix_jansson_schema_loader_fn loader,
                                                                        void* loader_ud,
                                                                        celix_jansson_schema_format_checker_fn format,
                                                                        void* format_ud,
                                                                        celix_jansson_schema_content_checker_fn content,
                                                                        void* content_ud) {
    celix_jansson_schema_validator_t* v = (celix_jansson_schema_validator_t*)calloc(1, sizeof(*v));
    if (!v)
        return NULL;
    v->root = (celix_jansson_schema_root_t*)calloc(1, sizeof(*v->root));
    if (!v->root) {
        free(v);
        return NULL;
    }
    celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
    opts.simpleRemovedCallback = celix_jansson_schema_file_free;
    v->root->files = celix_stringHashMap_createWithOptions(&opts); /* values freed via removed callback */
    if (!v->root->files) {
        free(v->root);
        free(v);
        return NULL;
    }
    v->root->loader = loader;
    v->root->loader_ud = loader_ud;
    v->root->format = format;
    v->root->format_ud = format_ud;
    v->root->content = content;
    v->root->content_ud = content_ud;
    return v;
}

void celix_jansson_schema_validator_destroy(celix_jansson_schema_validator_t* v) {
    if (!v)
        return;
    celix_jansson_schema_root_destroy(v->root);
    free(v);
}

void celix_jansson_schema_validator_set_abort_on_error(celix_jansson_schema_validator_t* v, bool enable) {
    if (v)
        v->abort_on_error = enable;
}

int celix_jansson_schema_set_root_schema(celix_jansson_schema_validator_t* v, json_t* schema, char** errmsg) {
    if (!v || !schema) {
        if (errmsg)
            *errmsg = strdup("validator and schema required");
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    celix_jansson_schema_root_t* root = v->root;
    celix_jansson_schema_unref(root->root);
    root->root = NULL;
    json_decref(root->original_schema);
    root->original_schema = NULL;
    celix_stringHashMap_clear(root->files); /* values freed via creation-time removed callback */

    json_t* copy = json_deep_copy(schema);
    if (!copy) {
        if (errmsg)
            *errmsg = strdup(celix_jansson_schema_strerror(CELIX_JANSSON_SCHEMA_ERROR_NOMEM));
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }

    root->original_schema = json_deep_copy(schema);
    if (!root->original_schema) {
        /* The first copy (L2446) is still owned by this frame; the decref at
         * the end of the compile path (after schema_make_internal_depth) is not
         * reached here, so release it explicitly. */
        json_decref(copy);
        if (errmsg)
            *errmsg = strdup(celix_jansson_schema_strerror(CELIX_JANSSON_SCHEMA_ERROR_NOMEM));
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }

    celix_jansson_uri_t root_base;
    celix_jansson_uri_init(&root_base, "");

    celix_jansson_schema_node_t* sch;
    int err = schema_make_internal_depth(copy, root, NULL, &root_base, &sch, 0);
    json_decref(copy);
    if (err != CELIX_JANSSON_SCHEMA_OK) {
        celix_jansson_uri_clear(&root_base);
        if (errmsg)
            *errmsg = strdup(celix_jansson_schema_strerror(err));
        return err;
    }

    root->root = sch;
    int rir = celix_jansson_schema_root_insert(root, &root_base, sch);
    if (rir == CELIX_JANSSON_SCHEMA_ERROR_NOMEM) {
        /* root_insert's NOMEM happens before its registry ref (L2256), so the
         * registry holds no reference; root->root still owns the only one.
         * Unref it and leave the root in the same clean state as the other
         * failing paths below (root->root == NULL). */
        celix_jansson_schema_unref(root->root);
        root->root = NULL;
        celix_jansson_uri_clear(&root_base);
        if (errmsg)
            *errmsg = strdup(celix_jansson_schema_strerror(CELIX_JANSSON_SCHEMA_ERROR_NOMEM));
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    /* DUPLICATE_URI is benign here: root->files was cleared above (L2444), so the
     * only way the "" key already exists is the top-level $id:"" registration done
     * during make (L1853); the registry already holds a reference in that case. */

    {
        char* rloc = celix_jansson_uri_location(&root_base);
        if (rloc) {
            celix_jansson_schema_file_t* rsf = celix_jansson_schema_root_get_or_create_file(root, rloc);
            if (rsf) {
                json_decref(rsf->document);
                rsf->document = json_incref(root->original_schema);
                /* strdup-then-swap: on OOM keep the old (content-identical) base_uri */
                char* nb = strdup(rloc);
                if (nb) {
                    free(rsf->base_uri);
                    rsf->base_uri = nb;
                }
            }
        }
        free(rloc);
    }
    celix_jansson_uri_clear(&root_base);

    err = resolve_external_refs(root);
    if (err != CELIX_JANSSON_SCHEMA_OK && errmsg)
        *errmsg = strdup(celix_jansson_schema_strerror(err));
    return err;
}

int celix_jansson_schema_validate(celix_jansson_schema_validator_t* v,
                                  json_t* instance,
                                  celix_jansson_schema_error_fn on_error,
                                  void* error_ud,
                                  json_t** patch_out) {
    if (!v)
        return -1;
    user_sink_t* sink = (user_sink_t*)calloc(1, sizeof(*sink));
    if (!sink)
        return -1;
    sink->base.emit = user_emit;
    sink->base.destroy = user_destroy;
    sink->fn = on_error;
    sink->ud = error_ud;

    celix_jansson_validation_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.root = v->root;
    ctx.sink = &sink->base;
    ctx.patch = json_array();

    sink->abort_on_error = v->abort_on_error;
    sink->ctx = &ctx;

    int errs = celix_jansson_schema_root_validate(v->root, "#", instance, &ctx);

    if (patch_out)
        *patch_out = ctx.patch;
    else
        json_decref(ctx.patch);

    sink->base.destroy(&sink->base);
    return errs;
}

int celix_jansson_schema_validate_uri(celix_jansson_schema_validator_t* v,
                                      json_t* instance,
                                      const char* initial_uri,
                                      celix_jansson_schema_error_fn on_error,
                                      void* error_ud,
                                      json_t** patch_out) {
    if (!v)
        return -1;
    user_sink_t* sink = (user_sink_t*)calloc(1, sizeof(*sink));
    if (!sink)
        return -1;
    sink->base.emit = user_emit;
    sink->base.destroy = user_destroy;
    sink->fn = on_error;
    sink->ud = error_ud;

    celix_jansson_validation_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.root = v->root;
    ctx.sink = &sink->base;
    ctx.patch = json_array();

    sink->abort_on_error = v->abort_on_error;
    sink->ctx = &ctx;

    int errs = celix_jansson_schema_root_validate(v->root, initial_uri ? initial_uri : "#", instance, &ctx);

    if (patch_out)
        *patch_out = ctx.patch;
    else
        json_decref(ctx.patch);

    sink->base.destroy(&sink->base);
    return errs;
}

json_t* celix_jansson_schema_draft7_meta_schema(void) {
    /* Full draft-07 meta-schema (from json-schema-org/JSON-Schema-Test-Suite,
     * as embedded in the reference json-schema-validator project).
     * A stub (type-only) here lets instances like {"minLength": -1} pass the
     * "remote ref, containing refs itself" suite cases that must be rejected. */
    const char* s = "{\"$schema\":\"http://json-schema.org/draft-07/schema#\","
        "\"$id\":\"http://json-schema.org/draft-07/schema#\",\"title\":\"Core schema meta-schema\","
        "\"definitions\":{\"schemaArray\":{\"type\":\"array\",\"minItems\":1,\"items\":{\"$ref\":\"#\"}},"
        "\"nonNegativeInteger\":{\"type\":\"integer\",\"minimum\":0},"
        "\"nonNegativeIntegerDefault0\":{\"allOf\":[{\"$ref\":\"#/definitions/nonNegativeInteger\"},"
        "{\"default\":0}]},\"simpleTypes\":{\"enum\":[\"array\",\"boolean\",\"integer\",\"null\",\"number\","
        "\"object\",\"string\"]},\"stringArray\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},"
        "\"uniqueItems\":true,\"default\":[]}},\"type\":[\"object\",\"boolean\"],"
        "\"properties\":{\"$id\":{\"type\":\"string\",\"format\":\"uri-reference\"},"
        "\"$schema\":{\"type\":\"string\",\"format\":\"uri\"},\"$ref\":{\"type\":\"string\","
        "\"format\":\"uri-reference\"},\"$comment\":{\"type\":\"string\"},\"title\":{\"type\":\"string\"},"
        "\"description\":{\"type\":\"string\"},\"default\":true,\"readOnly\":{\"type\":\"boolean\","
        "\"default\":false},\"examples\":{\"type\":\"array\",\"items\":true},"
        "\"multipleOf\":{\"type\":\"number\",\"exclusiveMinimum\":0},\"maximum\":{\"type\":\"number\"},"
        "\"exclusiveMaximum\":{\"type\":\"number\"},\"minimum\":{\"type\":\"number\"},"
        "\"exclusiveMinimum\":{\"type\":\"number\"},"
        "\"maxLength\":{\"$ref\":\"#/definitions/nonNegativeInteger\"},"
        "\"minLength\":{\"$ref\":\"#/definitions/nonNegativeIntegerDefault0\"},"
        "\"pattern\":{\"type\":\"string\",\"format\":\"regex\"},\"additionalItems\":{\"$ref\":\"#\"},"
        "\"items\":{\"anyOf\":[{\"$ref\":\"#\"},{\"$ref\":\"#/definitions/schemaArray\"}],\"default\":true},"
        "\"maxItems\":{\"$ref\":\"#/definitions/nonNegativeInteger\"},"
        "\"minItems\":{\"$ref\":\"#/definitions/nonNegativeIntegerDefault0\"},"
        "\"uniqueItems\":{\"type\":\"boolean\",\"default\":false},\"contains\":{\"$ref\":\"#\"},"
        "\"maxProperties\":{\"$ref\":\"#/definitions/nonNegativeInteger\"},"
        "\"minProperties\":{\"$ref\":\"#/definitions/nonNegativeIntegerDefault0\"},"
        "\"required\":{\"$ref\":\"#/definitions/stringArray\"},\"additionalProperties\":{\"$ref\":\"#\"},"
        "\"definitions\":{\"type\":\"object\",\"additionalProperties\":{\"$ref\":\"#\"},\"default\":{}},"
        "\"properties\":{\"type\":\"object\",\"additionalProperties\":{\"$ref\":\"#\"},\"default\":{}},"
        "\"patternProperties\":{\"type\":\"object\",\"additionalProperties\":{\"$ref\":\"#\"},"
        "\"propertyNames\":{\"format\":\"regex\"},\"default\":{}},\"dependencies\":{\"type\":\"object\","
        "\"additionalProperties\":{\"anyOf\":[{\"$ref\":\"#\"},{\"$ref\":\"#/definitions/stringArray\"}]}},"
        "\"propertyNames\":{\"$ref\":\"#\"},\"const\":true,\"enum\":{\"type\":\"array\",\"items\":true,"
        "\"minItems\":1,\"uniqueItems\":true},\"type\":{\"anyOf\":[{\"$ref\":\"#/definitions/simpleTypes\"},"
        "{\"type\":\"array\",\"items\":{\"$ref\":\"#/definitions/simpleTypes\"},\"minItems\":1,"
        "\"uniqueItems\":true}]},\"format\":{\"type\":\"string\"},\"contentMediaType\":{\"type\":\"string\"},"
        "\"contentEncoding\":{\"type\":\"string\"},\"if\":{\"$ref\":\"#\"},\"then\":{\"$ref\":\"#\"},"
        "\"else\":{\"$ref\":\"#\"},\"allOf\":{\"$ref\":\"#/definitions/schemaArray\"},"
        "\"anyOf\":{\"$ref\":\"#/definitions/schemaArray\"},"
        "\"oneOf\":{\"$ref\":\"#/definitions/schemaArray\"},\"not\":{\"$ref\":\"#\"}},\"default\":true}";
    return json_loads(s, 0, NULL);
}
