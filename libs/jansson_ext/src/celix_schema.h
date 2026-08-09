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
#ifndef CELIX_CELIX_SCHEMA_H
#define CELIX_CELIX_SCHEMA_H

#include "celix_jansson_uri.h"
#include "celix_string_hash_map.h"
#include "celix_util.h"
#include <regex.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Forward declarations ─────────────────────────────────────────────── */

typedef struct celix_jansson_schema_root_t celix_jansson_schema_root_t;
typedef struct celix_jansson_schema_node_t celix_jansson_schema_node_t;
typedef struct celix_jansson_error_sink_t celix_jansson_error_sink_t;
typedef struct celix_jansson_validation_context_t celix_jansson_validation_context_t;

/* ── Schema node kind enumeration ──────────────────────────────────────── */

enum celix_jansson_schema_kind_e {
    CELIX_JANSSON_SCHEMA_KIND_BOOLEAN,       /* true / false schema */
    CELIX_JANSSON_SCHEMA_KIND_TYPE,          /* type dispatcher */
    CELIX_JANSSON_SCHEMA_KIND_REF,           /* $ref proxy */
    CELIX_JANSSON_SCHEMA_KIND_STRING,        /* string constraints */
    CELIX_JANSSON_SCHEMA_KIND_NUMERIC_INT,   /* integer constraints */
    CELIX_JANSSON_SCHEMA_KIND_NUMERIC_FLOAT, /* number (float) constraints */
    CELIX_JANSSON_SCHEMA_KIND_NULL,          /* null type */
    CELIX_JANSSON_SCHEMA_KIND_BOOLEAN_TYPE,  /* boolean type */
    CELIX_JANSSON_SCHEMA_KIND_OBJECT,        /* object constraints */
    CELIX_JANSSON_SCHEMA_KIND_ARRAY,         /* array constraints */
    CELIX_JANSSON_SCHEMA_KIND_REQUIRED,      /* required property list (for array-form dependencies) */
    CELIX_JANSSON_SCHEMA_KIND_NOT,           /* "not" combinator */
    CELIX_JANSSON_SCHEMA_KIND_ALL_OF,        /* "allOf" combinator */
    CELIX_JANSSON_SCHEMA_KIND_ANY_OF,        /* "anyOf" combinator */
    CELIX_JANSSON_SCHEMA_KIND_ONE_OF,        /* "oneOf" combinator */
};

/* ── Path stack for recursive validation ───────────────────────────────── */

typedef struct celix_jansson_path_t {
    char** tokens;
    size_t len;
    size_t cap;
    char* cached; /* lazily computed pointer string */
} celix_jansson_path_t;

void celix_jansson_path_init(celix_jansson_path_t* p);
int celix_jansson_path_push(celix_jansson_path_t* p, const char* token);
void celix_jansson_path_pop(celix_jansson_path_t* p);
const char* celix_jansson_path_str(celix_jansson_path_t* p);
void celix_jansson_path_free(celix_jansson_path_t* p);

/* ── Vtable ────────────────────────────────────────────────────────────── */

struct celix_jansson_schema_node_t;

typedef struct schema_vtable {
    int (*validate)(const struct celix_jansson_schema_node_t* self,
                    json_t* instance,
                    celix_jansson_path_t* path,
                    struct celix_jansson_validation_context_t* ctx);
    const json_t* (*default_value)(const struct celix_jansson_schema_node_t* self,
                                   celix_jansson_path_t* path,
                                   const json_t* instance,
                                   struct celix_jansson_validation_context_t* ctx);
    void (*destroy)(struct celix_jansson_schema_node_t* self);
} schema_vtable;

/* ── Schema node ───────────────────────────────────────────────────────── */

struct celix_jansson_schema_node_t {
    const schema_vtable* vtable;
    enum celix_jansson_schema_kind_e kind;
    unsigned refcount;
    celix_jansson_schema_root_t* root;
    json_t* default_value; /* owned, or NULL */

    union {
        /* CELIX_JANSSON_SCHEMA_KIND_BOOLEAN */
        struct {
            bool value; /* true = always valid */
        } boolean;

        /* CELIX_JANSSON_SCHEMA_KIND_TYPE */
        struct {
            /*
             * One slot per JSON type. Index:
             *   0 = null, 1 = object, 2 = array, 3 = string,
             *   4 = boolean, 5 = integer, 6 = real
             */
            celix_jansson_schema_node_t* type_slots[7];
            /* enum */
            bool has_enum;
            json_t* enum_values; /* array, owned */
            /* const */
            bool has_const;
            json_t* const_value; /* owned */
            /* logical combinators (not/allOf/anyOf/oneOf, in order) */
            celix_jansson_schema_node_t** logic;
            size_t logic_len;
            /* conditional */
            celix_jansson_schema_node_t* if_schema;
            celix_jansson_schema_node_t* then_schema;
            celix_jansson_schema_node_t* else_schema;
        } type_schema;

        /* CELIX_JANSSON_SCHEMA_KIND_REF */
        struct {
            char* id;                                   /* the $ref URI string */
            celix_jansson_schema_node_t* target_weak;   /* non-owning (registry holds ref) */
        } ref;

        /* CELIX_JANSSON_SCHEMA_KIND_STRING */
        struct {
            bool has_min_len, has_max_len;
            size_t min_len, max_len;
            bool has_pattern;
            regex_t pattern;
            char* pattern_str; /* original pattern string */
            bool has_format;
            char* format;
            bool has_content;
            char* content_encoding;
            char* content_media_type;
        } string;

        /* CELIX_JANSSON_SCHEMA_KIND_NUMERIC_INT / CELIX_JANSSON_SCHEMA_KIND_NUMERIC_FLOAT */
        struct {
            bool has_max, has_min, has_mult;
            bool exclusive_max, exclusive_min;
            double multiple_of; /* always double for arithmetic */
            /* Use union for storage; kind determines which is active */
            union {
                struct {
                    json_int_t max;
                    json_int_t min;
                } i;
                struct {
                    double max;
                    double min;
                } f;
            } bounds;
        } numeric;

        /* CELIX_JANSSON_SCHEMA_KIND_OBJECT */
        struct {
            bool has_min_p, has_max_p;
            size_t min_p, max_p;
            char** required; /* array of required property names */
            size_t required_len;
            celix_string_hash_map_t* properties; /* name -> celix_jansson_schema_node_t* (owning ref) */
            /* patternProperties: array of (compiled regex, celix_jansson_schema_node_t*) */
            struct {
                regex_t re;
                celix_jansson_schema_node_t* sch;
            }* pattern_properties;
            size_t pp_len;
            celix_jansson_schema_node_t* additional_properties; /* or NULL */
            celix_string_hash_map_t* dependencies;            /* name -> celix_jansson_schema_node_t* (owning ref,
                                                                   CELIX_JANSSON_SCHEMA_KIND_REQUIRED or full schema) */
            celix_jansson_schema_node_t* property_names;        /* or NULL */
        } object;

        /* CELIX_JANSSON_SCHEMA_KIND_ARRAY */
        struct {
            bool has_min_i, has_max_i;
            size_t min_items, max_items;
            bool unique_items;
            /* items: single-schema form */
            celix_jansson_schema_node_t* items_schema;
            /* items: tuple form (mutually exclusive with items_schema) */
            celix_jansson_schema_node_t** items;
            size_t items_len;
            celix_jansson_schema_node_t* additional_items;
            celix_jansson_schema_node_t* contains;
        } array;

        /* CELIX_JANSSON_SCHEMA_KIND_REQUIRED */
        struct {
            char** names;
            size_t len;
        } required;

        /* CELIX_JANSSON_SCHEMA_KIND_NOT */
        struct {
            celix_jansson_schema_node_t* sub;
        } not_schema;

        /* CELIX_JANSSON_SCHEMA_KIND_ALL_OF / ANY_OF / ONE_OF */
        struct {
            celix_jansson_schema_node_t** items;
            size_t len;
        } combination;
    } u;
};

/* ── Reference counting ────────────────────────────────────────────────── */

/** Increment refcount. Returns the node (for chaining). */
celix_jansson_schema_node_t* celix_jansson_schema_ref(celix_jansson_schema_node_t* n);

/** Decrement refcount. Frees the node (and all children) when zero. */
void celix_jansson_schema_unref(celix_jansson_schema_node_t* n);

/* ── Auto cleanup ──────────────────────────────────────────────────────── */

/** Enables `celix_autoptr(celix_jansson_schema_node_t)` for scope-based
 *  ownership of schema nodes (NULL-safe; routes through the vtable destroy). */
CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_jansson_schema_node_t, celix_jansson_schema_unref)

/* ── Error sink (polymorphic error collector) ──────────────────────────── */

struct celix_jansson_error_sink_t {
    void (*emit)(struct celix_jansson_error_sink_t* sink, const char* path_str, json_t* instance, const char* message);
    void (*destroy)(struct celix_jansson_error_sink_t* sink);
    void* data;
};

/* ── Error entry for collecting sinks ──────────────────────────────────── */

typedef struct celix_jansson_error_entry_t {
    char* ptr;        /* malloc'd JSON pointer string */
    json_t* instance; /* json_incref'd */
    char* message;    /* malloc'd error message */
} celix_jansson_error_entry_t;

typedef struct celix_jansson_error_list_t {
    celix_jansson_error_entry_t* entries;
    size_t len;
    size_t cap;
} celix_jansson_error_list_t;

void celix_jansson_error_list_init(celix_jansson_error_list_t* el);
void celix_jansson_error_list_add(celix_jansson_error_list_t* el, const char* ptr, json_t* instance, const char* msg);
void celix_jansson_error_list_clear(celix_jansson_error_list_t* el);

/* ── Validation context ────────────────────────────────────────────────── */

struct celix_jansson_validation_context_t {
    celix_jansson_schema_root_t* root;
    celix_jansson_error_sink_t* sink;
    json_t* patch;                  /* owned JSON array of {op,path,value} objects */
    celix_jansson_strbuf_t scratch; /* reusable message formatting buffer */
    bool aborted;                   /* set by abort-on-error sink; checked by validators to stop early */
    int ref_depth;                  /* guards against infinite recursion through circular $ref */
};

/* ── Schema file (per-location registry entry) ─────────────────────────── */

typedef struct celix_jansson_schema_file_t {
    celix_string_hash_map_t* schemas; /* fragment string -> celix_jansson_schema_node_t* (owning ref) */
    celix_string_hash_map_t*
        unresolved; /* fragment string -> celix_jansson_schema_node_t* (CELIX_JANSSON_SCHEMA_KIND_REF, owning ref) */
    json_t* document;        /* original JSON document for this location (owned), or NULL */
    char* base_uri;          /* base URI used when compiling this document (owned), or NULL */
    celix_jansson_vec_t retained; /* resolved placeholders kept alive for weak-ref indirection */
} celix_jansson_schema_file_t;

/* ── Root schema (the central registry) ────────────────────────────────── */

struct celix_jansson_schema_root_t {
    celix_string_hash_map_t* files; /* location string -> celix_jansson_schema_file_t* */
    celix_jansson_schema_loader_fn loader;
    void* loader_ud;
    celix_jansson_schema_format_checker_fn format;
    void* format_ud;
    celix_jansson_schema_content_checker_fn content;
    void* content_ud;
    celix_jansson_schema_node_t* root; /* the compiled root, or NULL */
    json_t* original_schema;           /* original JSON for JSON-pointer $ref resolution */
};

/* ── Registry functions ────────────────────────────────────────────────── */

celix_jansson_schema_file_t* celix_jansson_schema_root_get_or_create_file(celix_jansson_schema_root_t* root,
                                                                          const char* location);
int celix_jansson_schema_root_insert(celix_jansson_schema_root_t* root,
                                     const celix_jansson_uri_t* uri,
                                     celix_jansson_schema_node_t* sch);
int celix_jansson_schema_root_validate(celix_jansson_schema_root_t* root,
                                       const char* initial_uri,
                                       json_t* instance,
                                       celix_jansson_validation_context_t* ctx);
void celix_jansson_schema_root_destroy(celix_jansson_schema_root_t* root);

/* ── Type mapping ──────────────────────────────────────────────────────── */

/** Map a jansson json_type to the type_slots index (0..6). Returns -1 if unknown. */
int celix_jansson_type_index(json_t* value);

#endif /* CELIX_CELIX_SCHEMA_H */
