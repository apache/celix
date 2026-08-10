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
#include "celix_json_patch.h"
#include "celix_jansson_schema.h"
#include "celix_jansson_pointer.h"
#include "celix_cleanup.h"
#include <stdlib.h>
#include <string.h>

int celix_json_patch_add(json_t* patch, const char* path_str, json_t* value) {
    if (!patch || !json_is_array(patch))
        return -1;

    /* val/op are auto-released on every early return */
    json_auto_t* val = value;
    json_auto_t* op = json_object();
    if (!op)
        return -1;

    json_auto_t* op_name = json_string("add");
    if (!op_name)
        return -1;
    if (json_object_set_new(op, "op", celix_steal_ptr(op_name)) != 0)
        return -1;

    json_auto_t* path = json_string(path_str);
    if (!path)
        return -1;
    if (json_object_set_new(op, "path", celix_steal_ptr(path)) != 0)
        return -1;

    if (json_object_set_new(op, "value", celix_steal_ptr(val)) != 0)
        return -1;

    /* on failure the op (and with it the value) is consumed */
    return json_array_append_new(patch, celix_steal_ptr(op));
}

int celix_json_patch_replace(json_t* patch, const char* path_str, json_t* value) {
    if (!patch || !json_is_array(patch))
        return -1;

    /* val/op are auto-released on every early return */
    json_auto_t* val = value;
    json_auto_t* op = json_object();
    if (!op)
        return -1;

    json_auto_t* op_name = json_string("replace");
    if (!op_name)
        return -1;
    if (json_object_set_new(op, "op", celix_steal_ptr(op_name)) != 0)
        return -1;

    json_auto_t* path = json_string(path_str);
    if (!path)
        return -1;
    if (json_object_set_new(op, "path", celix_steal_ptr(path)) != 0)
        return -1;

    if (json_object_set_new(op, "value", celix_steal_ptr(val)) != 0)
        return -1;

    /* on failure the op (and with it the value) is consumed */
    return json_array_append_new(patch, celix_steal_ptr(op));
}

int celix_json_patch_remove(json_t* patch, const char* path_str) {
    if (!patch || !json_is_array(patch))
        return -1;

    /* op is auto-released on every early return */
    json_auto_t* op = json_object();
    if (!op)
        return -1;

    json_auto_t* op_name = json_string("remove");
    if (!op_name)
        return -1;
    if (json_object_set_new(op, "op", celix_steal_ptr(op_name)) != 0)
        return -1;

    json_auto_t* path = json_string(path_str);
    if (!path)
        return -1;
    if (json_object_set_new(op, "path", celix_steal_ptr(path)) != 0)
        return -1;

    /* on failure the op is consumed */
    return json_array_append_new(patch, celix_steal_ptr(op));
}

void celix_json_patch_truncate(json_t* patch, size_t old_size) {
    if (!patch || !json_is_array(patch))
        return;

    while (json_array_size(patch) > old_size)
        json_array_remove(patch, json_array_size(patch) - 1);
}

/* ── Patch application ─────────────────────────────────────────────────── */

json_t* celix_json_patch_apply(json_t* original, json_t* patch) {
    if (!original || !patch || !json_is_array(patch))
        return NULL;

    /* result is auto-released on the error path */
    json_auto_t* result = json_deep_copy(original);
    if (!result)
        return NULL;

    size_t n = json_array_size(patch);
    for (size_t i = 0; i < n; i++) {
        json_t* op_obj = json_array_get(patch, i);
        const char* op_type = json_string_value(json_object_get(op_obj, "op"));
        const char* path_str = json_string_value(json_object_get(op_obj, "path"));
        json_t* value = json_object_get(op_obj, "value");

        if (!op_type || !path_str)
            continue;

        /* Parse the path; ptr is auto-cleared when the loop body exits,
         * including on continue and on the error jump below */
        celix_auto(celix_json_pointer_t) ptr;
        if (celix_json_pointer_init(&ptr, path_str) != 0)
            continue;

        if (strcmp(op_type, "add") == 0 || strcmp(op_type, "replace") == 0) {
            if (!value)
                continue;

            if (ptr.len == 0) {
                /* Root replacement: copy first, then swap the ownership */
                json_decref(result);
                result = json_deep_copy(value);
                if (!result)
                    return NULL;
            } else {
                /* Find parent */
                const char* last = ptr.tokens[ptr.len - 1];
                /* Walk to parent, creating intermediate nodes */
                json_t* parent = result;
                for (size_t j = 0; j < ptr.len - 1; j++) {
                    json_t* child = NULL;
                    if (json_is_object(parent)) {
                        child = json_object_get(parent, ptr.tokens[j]);
                        if (!child) {
                            child = json_object();
                            if (!child)
                                return NULL;
                            /* child is consumed by a failed set_new */
                            if (json_object_set_new(parent, ptr.tokens[j], child) != 0)
                                return NULL;
                        }
                    } else if (json_is_array(parent)) {
                        char* end;
                        long idx = strtol(ptr.tokens[j], &end, 10);
                        if (*end != '\0')
                            break;
                        while ((size_t)idx > json_array_size(parent)) {
                            json_t* pad = json_null();
                            if (!pad)
                                return NULL;
                            /* pad is consumed by a failed append */
                            if (json_array_append_new(parent, pad) != 0)
                                return NULL;
                        }
                        child = json_array_get(parent, (size_t)idx);
                        if (!child) {
                            child = json_object();
                            if (!child)
                                return NULL;
                            if (json_array_append_new(parent, child) != 0)
                                return NULL;
                        }
                    }
                    parent = child;
                    if (!parent)
                        break;
                }

                if (parent && json_is_object(parent)) {
                    if (strcmp(op_type, "replace") == 0 && json_object_get(parent, last)) {
                        /* the incref'd copy is consumed by a failed set_new */
                        if (json_object_set_new(parent, last, json_incref(value)) != 0)
                            return NULL;
                    } else if (strcmp(op_type, "add") == 0) {
                        /* the incref'd copy is consumed by a failed set_new */
                        if (json_object_set_new(parent, last, json_incref(value)) != 0)
                            return NULL;
                    }
                } else if (parent && json_is_array(parent)) {
                    char* end;
                    long idx = strtol(last, &end, 10);
                    if (*end == '\0' && idx >= 0) {
                        if (strcmp(op_type, "replace") == 0 && (size_t)idx < json_array_size(parent)) {
                            json_t* copy = json_deep_copy(value);
                            if (!copy)
                                return NULL;
                            /* copy is consumed by a failed set_new */
                            if (json_array_set_new(parent, (size_t)idx, copy) != 0)
                                return NULL;
                        } else if (strcmp(op_type, "add") == 0) {
                            while ((size_t)idx > json_array_size(parent)) {
                                json_t* pad = json_null();
                                if (!pad)
                                    return NULL;
                                if (json_array_append_new(parent, pad) != 0)
                                    return NULL;
                            }
                            if ((size_t)idx == json_array_size(parent)) {
                                /* the incref'd copy is consumed by a failed append */
                                if (json_array_append_new(parent, json_incref(value)) != 0)
                                    return NULL;
                            } else {
                                json_t* copy = json_deep_copy(value);
                                if (!copy)
                                    return NULL;
                                /* copy is consumed by a failed insert_new */
                                if (json_array_insert_new(parent, (size_t)idx, copy) != 0)
                                    return NULL;
                            }
                        }
                    }
                }
            }
        } else if (strcmp(op_type, "remove") == 0) {
            if (ptr.len == 0) {
                /* Root removal: create the null first, then swap the ownership */
                json_decref(result);
                result = json_null();
                if (!result)
                    return NULL;
            } else {
                const char* last = ptr.tokens[ptr.len - 1];
                json_t* parent = result;
                for (size_t j = 0; j < ptr.len - 1 && parent; j++) {
                    if (json_is_object(parent))
                        parent = json_object_get(parent, ptr.tokens[j]);
                    else if (json_is_array(parent)) {
                        char* end;
                        long idx = strtol(ptr.tokens[j], &end, 10);
                        parent = (*end == '\0' && idx >= 0) ? json_array_get(parent, (size_t)idx) : NULL;
                    } else
                        parent = NULL;
                }
                if (parent && json_is_object(parent))
                    json_object_del(parent, last);
                else if (parent && json_is_array(parent)) {
                    char* end;
                    long idx = strtol(last, &end, 10);
                    if (*end == '\0' && idx >= 0 && (size_t)idx < json_array_size(parent))
                        json_array_remove(parent, (size_t)idx);
                }
            }
        }
    }

    return celix_steal_ptr(result);
}
