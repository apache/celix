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
#ifndef CELIX_CELIX_JANSSON_URI_H
#define CELIX_CELIX_JANSSON_URI_H

#include <stdbool.h>

#include "celix_jansson_schema.h"
#include "celix_jansson_pointer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── URI structure ─────────────────────────────────────────────────────── */

typedef struct celix_jansson_uri_t {
    char* urn;                    /* "urn:..." or NULL */
    char* scheme;                 /* e.g., "http" */
    char* authority;              /* e.g., "json-schema.org" */
    char* path;                   /* e.g., "/draft-07/schema" */
    celix_json_pointer_t pointer; /* fragment, when it starts with '/' */
    char* identifier;             /* fragment, when it's a plain-name identifier */
} celix_jansson_uri_t;

/** Initialize/parse a URI from a string. Returns JSS error code. */
int celix_jansson_uri_init(celix_jansson_uri_t* u, const char* uri_str);

/** Update a URI in-place by resolving @p uri_str against it. */
int celix_jansson_uri_update(celix_jansson_uri_t* u, const char* uri_str);

/**
 * Derive a new URI by resolving @p uri_str relative to @p base.
 * @p out must be zero-initialized (or a fresh celix_jansson_uri_init result).
 */
int celix_jansson_uri_derive(const celix_jansson_uri_t* base, const char* uri_str, celix_jansson_uri_t* out);

/**
 * Append a JSON Pointer token to the URI (no-op if the URI has an
 * identifier fragment).
 */
int celix_jansson_uri_append(const celix_jansson_uri_t* u, const char* token, celix_jansson_uri_t* out);

/** Reconstruct the location part (scheme://authority/path or URN). Returns malloc'd string; caller must free(). */
char* celix_jansson_uri_location(const celix_jansson_uri_t* u);

/** Full URI string "location # fragment".  Returns malloc'd string. */
char* celix_jansson_uri_to_string(const celix_jansson_uri_t* u);

/** Escape special chars for JSON Pointer (~ and /). Returns malloc'd. */
char* celix_jansson_uri_escape(const char* src);

/** Return the fragment as a string (concatenation of pointer or identifier). Returns malloc'd string; caller must free(). */
char* celix_jansson_uri_fragment(const celix_jansson_uri_t* u);

/** Compare two URIs for equality. Returns true if equal. */
bool celix_jansson_uri_equals(const celix_jansson_uri_t* a, const celix_jansson_uri_t* b);

/** Free all memory held by a URI. */
void celix_jansson_uri_clear(celix_jansson_uri_t* u);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_CELIX_JANSSON_URI_H */
