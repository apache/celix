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
#include "celix_jansson_uri.h"
#include "celix_stdlib_cleanup.h"
#include "celix_util.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── URI ──────────────────────────────────────────────────────────────── */

/* Percent-decode a string. Returns malloc'd output. */
static char* percent_decode(const char* src) {
    celix_auto(celix_jansson_strbuf_t) sb;
    celix_jansson_strbuf_init(&sb);

    for (const char* s = src; *s; s++) {
        int rc;
        if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) {
            unsigned int val;
            char hex[3] = {s[1], s[2], '\0'};
            sscanf(hex, "%2x", &val);
            rc = celix_jansson_strbuf_appendc(&sb, (char)val);
            s += 2;
        } else {
            rc = celix_jansson_strbuf_appendc(&sb, *s);
        }
        if (rc != 0) {
            /* OOM: sb is released automatically; update() maps NULL to NOMEM */
            return NULL;
        }
    }

    return celix_jansson_strbuf_detach(&sb);
}

int celix_jansson_uri_init(celix_jansson_uri_t* u, const char* uri_str) {
    memset(u, 0, sizeof(*u));
    int rc = celix_jansson_uri_update(u, uri_str);
    if (rc != 0)
        celix_jansson_uri_clear(u); /* leave u fully cleared on failure */
    return rc;
}

int celix_jansson_uri_update(celix_jansson_uri_t* u, const char* uri_str) {
    if (!uri_str)
        return 0;

    /* Split at first '#' */
    const char* hash = strchr(uri_str, '#');
    size_t loc_len = hash ? (size_t)(hash - uri_str) : strlen(uri_str);
    bool fragment_only = (loc_len == 0 && hash != NULL);
    bool has_scheme = (loc_len > 0 && strstr(uri_str, "://") != NULL);

    /* Save old components for relative resolution. Only needed for relative
     * refs (no scheme, non-fragment-only); each strdup is checked individually
     * so no OOM goes unhandled — u is not yet modified here. The buffers are
     * freed automatically on every exit path (celix_autofree). */
    celix_autofree char* old_path = NULL;
    celix_autofree char* old_scheme = NULL;
    celix_autofree char* old_authority = NULL;
    if (loc_len > 0 && !has_scheme) {
        old_path = u->path ? strdup(u->path) : NULL;
        if (u->path && !old_path)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases old-* */
        old_scheme = u->scheme ? strdup(u->scheme) : NULL;
        if (u->scheme && !old_scheme)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases old-* */
        old_authority = u->authority ? strdup(u->authority) : NULL;
        if (u->authority && !old_authority)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases old-* */
    }

    /* Fragment-only refs keep the location, only update fragment */
    if (fragment_only) {
        /* Only clear the fragment part */
        free(u->identifier);
        u->identifier = NULL;
        celix_json_pointer_clear(&u->pointer);
    } else {
        /* Free ALL existing components */
        celix_jansson_uri_clear(u);
        /* Restore scheme/authority for relative paths that don't replace them */
        if (old_scheme) {
            u->scheme = celix_steal_ptr(old_scheme);
        }
        if (old_authority) {
            u->authority = celix_steal_ptr(old_authority);
        }
    }

    /* Decode the location part (before '#'); freed automatically on every
     * exit path (celix_autofree) unless ownership is transferred to u->urn */
    celix_autofree char* location = NULL;
    if (loc_len > 0) {
        location = (char*)malloc(loc_len + 1);
        if (!location)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        memcpy(location, uri_str, loc_len);
        location[loc_len] = '\0';
    }

    /* Decode the fragment (after '#') */
    if (hash && hash[1]) {
        const char* frag = hash + 1;

        /* Percent-decode the fragment */
        celix_autofree char* decoded = percent_decode(frag);
        if (!decoded)
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */

        /* Fragment classification */
        if (decoded[0] == '/') {
            /* JSON Pointer */
            if (celix_json_pointer_init(&u->pointer, decoded) != 0) {
                return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
            }
        } else {
            /* Plain-name identifier */
            u->identifier = celix_steal_ptr(decoded);
        }
    }

    /* Parse location */
    if (location) {
        /* Check for URN */
        if (strncmp(location, "urn:", 4) == 0) {
            u->urn = celix_steal_ptr(location);
        } else {
            /* URL parsing */
            const char* p = location;
            /* Scheme */
            const char* colon = strstr(p, "://");
            if (colon) {
                size_t scheme_len = (size_t)(colon - p);
                u->scheme = (char*)malloc(scheme_len + 1);
                if (!u->scheme)
                    return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                memcpy(u->scheme, p, scheme_len);
                u->scheme[scheme_len] = '\0';
                p = colon + 3;
                /* Authority */
                const char* slash = strchr(p, '/');
                if (slash) {
                    size_t auth_len = (size_t)(slash - p);
                    u->authority = (char*)malloc(auth_len + 1);
                    if (!u->authority)
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                    memcpy(u->authority, p, auth_len);
                    u->authority[auth_len] = '\0';
                    u->path = strdup(slash);
                    if (!u->path)
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                } else {
                    u->authority = strdup(p);
                    if (!u->authority)
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                    u->path = NULL;
                }
            } else {
                /* Relative path (no scheme://) */
                if (p[0] == '/') {
                    /* Absolute path */
                    u->path = strdup(p);
                    if (!u->path)
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                } else if (old_path && *p) {
                    /* Resolve relative to old path's directory */
                    const char* last_slash = strrchr(old_path, '/');
                    if (last_slash) {
                        size_t dir_len = (size_t)(last_slash - old_path);
                        celix_auto(celix_jansson_strbuf_t) sb;
                        celix_jansson_strbuf_init(&sb);
                        if (celix_jansson_strbuf_append(&sb, old_path, dir_len) != 0 ||
                            celix_jansson_strbuf_appendc(&sb, '/') != 0 ||
                            celix_jansson_strbuf_appends(&sb, p) != 0) {
                            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* sb released automatically */
                        }
                        u->path = celix_jansson_strbuf_detach(&sb);
                    } else {
                        u->path = strdup(p);
                        if (!u->path)
                            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                    }
                } else {
                    u->path = strdup(p);
                    if (!u->path)
                        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM; /* autofree releases location/old-* */
                }
            }
        }
    }

    return 0;
}

int celix_jansson_uri_derive(const celix_jansson_uri_t* base, const char* uri_str, celix_jansson_uri_t* out) {
    /* Start with a copy of the base */
    celix_jansson_uri_clear(out);

    /* Copy base components; each strdup is checked individually so no OOM
     * goes unhandled (on failure out is left fully cleared) */
    out->scheme = base->scheme ? strdup(base->scheme) : NULL;
    if (base->scheme && !out->scheme) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    out->authority = base->authority ? strdup(base->authority) : NULL;
    if (base->authority && !out->authority) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    out->path = base->path ? strdup(base->path) : NULL;
    if (base->path && !out->path) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    out->urn = base->urn ? strdup(base->urn) : NULL;
    if (base->urn && !out->urn) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    if (base->identifier) {
        out->identifier = strdup(base->identifier);
        if (!out->identifier) {
            celix_jansson_uri_clear(out);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
    } else {
        /* Copy pointer tokens */
        for (size_t i = 0; i < base->pointer.len; i++) {
            if (celix_json_pointer_push(&out->pointer, base->pointer.tokens[i]) != 0) {
                celix_jansson_uri_clear(out);
                return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
            }
        }
    }

    /* Now resolve uri_str relative to this */
    int rc = celix_jansson_uri_update(out, uri_str);
    if (rc != 0)
        celix_jansson_uri_clear(out); /* release the copied base components on failure */
    return rc;
}

int celix_jansson_uri_append(const celix_jansson_uri_t* u, const char* token, celix_jansson_uri_t* out) {
    /* Start with a copy; each strdup is checked individually so no OOM goes
     * unhandled (on failure out is left fully cleared) */
    celix_jansson_uri_clear(out);
    out->scheme = u->scheme ? strdup(u->scheme) : NULL;
    if (u->scheme && !out->scheme) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    out->authority = u->authority ? strdup(u->authority) : NULL;
    if (u->authority && !out->authority) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    out->path = u->path ? strdup(u->path) : NULL;
    if (u->path && !out->path) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }
    out->urn = u->urn ? strdup(u->urn) : NULL;
    if (u->urn && !out->urn) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }

    if (u->identifier) {
        out->identifier = strdup(u->identifier);
        if (!out->identifier) {
            celix_jansson_uri_clear(out);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
        return 0; /* no-op for identifier URIs */
    }

    /* Copy existing pointer tokens */
    for (size_t i = 0; i < u->pointer.len; i++) {
        if (celix_json_pointer_push(&out->pointer, u->pointer.tokens[i]) != 0) {
            celix_jansson_uri_clear(out);
            return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
        }
    }
    /* Append new token */
    if (celix_json_pointer_push(&out->pointer, token) != 0) {
        celix_jansson_uri_clear(out);
        return CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }

    return 0;
}

char* celix_jansson_uri_location(const celix_jansson_uri_t* u) {
    if (u->urn) {
        return strdup(u->urn); /* NULL only on allocation failure (OOM signal) */
    }

    celix_auto(celix_jansson_strbuf_t) sb;
    celix_jansson_strbuf_init(&sb);
    if (u->scheme) {
        if (celix_jansson_strbuf_appends(&sb, u->scheme) != 0 ||
            celix_jansson_strbuf_appends(&sb, "://") != 0)
            return NULL; /* OOM signal; sb released automatically */
    }
    if (u->authority && celix_jansson_strbuf_appends(&sb, u->authority) != 0)
        return NULL; /* OOM signal */
    if (u->path && celix_jansson_strbuf_appends(&sb, u->path) != 0)
        return NULL; /* OOM signal */

    if (sb.len == 0)
        return strdup(""); /* NULL only on allocation failure */

    return celix_jansson_strbuf_detach(&sb); /* never NULL when len > 0 */
}

char* celix_jansson_uri_to_string(const celix_jansson_uri_t* u) {
    celix_autofree char* loc = celix_jansson_uri_location(u);
    celix_autofree char* frag = celix_jansson_uri_fragment(u);

    celix_auto(celix_jansson_strbuf_t) sb;
    celix_jansson_strbuf_init(&sb);
    /* OOM: any allocation failure below propagates as NULL (never a crash);
     * loc/frag/sb are released automatically on every exit path */
    if (!loc || !frag)
        return NULL; /* OOM signal */
    if (celix_jansson_strbuf_appends(&sb, loc) != 0)
        return NULL; /* OOM signal */
    if (*frag) {
        if (celix_jansson_strbuf_appends(&sb, "#") != 0 ||
            celix_jansson_strbuf_appends(&sb, frag) != 0)
            return NULL; /* OOM signal */
    }

    if (sb.len == 0)
        return strdup(""); /* NULL only on allocation failure */

    return celix_jansson_strbuf_detach(&sb);
}

char* celix_jansson_uri_escape(const char* src) {
    celix_auto(celix_jansson_strbuf_t) sb;
    celix_jansson_strbuf_init(&sb);

    for (const char* c = src; *c; c++) {
        int rc;
        if (*c == '~')
            rc = celix_jansson_strbuf_appends(&sb, "~0");
        else if (*c == '/')
            rc = celix_jansson_strbuf_appends(&sb, "~1");
        else
            rc = celix_jansson_strbuf_appendc(&sb, *c);
        if (rc != 0)
            return NULL; /* OOM signal; sb released automatically */
    }

    if (sb.len == 0)
        return strdup(""); /* escape("") must return "" rather than NULL */

    return celix_jansson_strbuf_detach(&sb);
}

char* celix_jansson_uri_fragment(const celix_jansson_uri_t* u) {
    /* Returns NULL only on allocation failure (OOM signal); there is no
     * internal crash path. Callers must check for NULL. */
    if (u->identifier)
        return strdup(u->identifier);

    if (u->pointer.len > 0) {
        return celix_json_pointer_to_string(&u->pointer);
    }

    return strdup("");
}

bool celix_jansson_uri_equals(const celix_jansson_uri_t* a, const celix_jansson_uri_t* b) {
    celix_autofree char* la = celix_jansson_uri_location(a);
    celix_autofree char* lb = celix_jansson_uri_location(b);
    if (!la || !lb) { /* OOM: degrade to "not equal" instead of strcmp(NULL) */
        return false;
    }
    int r = strcmp(la, lb);
    if (r != 0)
        return false;
    celix_autofree char* fa = celix_jansson_uri_fragment(a);
    celix_autofree char* fb = celix_jansson_uri_fragment(b);
    if (!fa || !fb) {
        return false;
    }
    r = strcmp(fa, fb);
    return r == 0;
}

void celix_jansson_uri_clear(celix_jansson_uri_t* u) {
    free(u->urn);
    free(u->scheme);
    free(u->authority);
    free(u->path);
    free(u->identifier);
    celix_json_pointer_clear(&u->pointer);
    memset(u, 0, sizeof(*u));
}
