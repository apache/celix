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
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "json_serializer.h"
#include "dyn_type.h"
#include "dyn_type_common.h"
#include "dyn_interface.h"
#include "celix_err.h"

#include <jansson.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

static int jsonSerializer_createType(const dyn_type* type, json_t* object, void** result);
static int jsonSerializer_parseObject(const dyn_type* type, json_t* object, void* inst);
static int jsonSerializer_parseSequence(const dyn_type* seq, json_t* array, void* seqLoc);
static int jsonSerializer_parseAny(const dyn_type* type, void* input, json_t* val);
static int jsonSerializer_parseEnum(const dyn_type* type, const char* enum_name, int32_t* out);

static int jsonSerializer_writeAny(const dyn_type* type, void* input, json_t** val);
static int jsonSerializer_writeComplex(const dyn_type* type, void* input, json_t** val);
static int jsonSerializer_writeSequence(const dyn_type* type, void* input, json_t** out);
static int jsonSerializer_writeEnum(const dyn_type* type, int32_t enum_value, json_t** out);


static int OK = 0;
static int ERROR = 1;

int jsonSerializer_deserialize(const dyn_type* type, const char* input, size_t length, void** result) {
    int status = 0;

    json_error_t error;
    json_auto_t* root = json_loadb(input, length, JSON_DECODE_ANY, &error);
    if (root == NULL) {
        celix_err_pushf("Error parsing json input '%.*s'. Error is: %s\n", (int)length, input, error.text);
        return ERROR;
    }

    status = jsonSerializer_deserializeJson(type, root, result);
    if (status != OK) {
        celix_err_pushf("Error cannot deserialize json. Input is '%s'", input);
    }
    return status;
}

int jsonSerializer_deserializeJson(const dyn_type* type, json_t* input, void** out) {
    return jsonSerializer_createType(type, input, out);
}

static int jsonSerializer_createType(const dyn_type* type, json_t* val, void** result) {
    assert(val != NULL);
    int status = OK;
    void* inst = NULL;

    if ((status = dynType_alloc(type, &inst)) != OK) {
        return status;
    }

    if ((status = jsonSerializer_parseAny(type, inst, val)) != OK) {
        dynType_free(type, inst);
        *result = NULL;
        return status;
    }

    *result = inst;
    return OK;
}

static int jsonSerializer_parseObject(const dyn_type* type, json_t* object, void* inst) {
    assert(object != NULL);
    int status = OK;
    json_t* value;
    const struct complex_type_entries_head* entries = dynType_complex_entries(type);
    struct complex_type_entry* entry = NULL;
    int index = 0;
    void* valp = NULL;
    const dyn_type* valType = NULL;
    TAILQ_FOREACH(entry, entries, entries) {
        if (entry->name == NULL) {
            celix_err_push("Unamed field unsupported");
            return ERROR;
        }
        value = json_object_get(object, entry->name);
        if (value == NULL) {
            celix_err_pushf("Missing object member %s", entry->name);
            return ERROR;
        }
        valp = dynType_complex_valLocAt(type, index, inst);
        valType = dynType_complex_dynTypeAt(type, index);
        status = jsonSerializer_parseAny(valType, valp, value);
        if (status != OK) {
            break;
        }
        index++;
    }

    return status;
}

static int jsonSerializer_parseAny(const dyn_type* type, void* loc, json_t* val) {
    int status = OK;

    const dyn_type* subType = NULL;
    char c = dynType_descriptorType(type);

    switch (c) {
        case 'Z' :
            *(bool*)loc = (bool) json_is_true(val);
            break;
        case 'F' :
            *(float*)loc = (float) json_real_value(val);
            break;
        case 'D' :
            *(double*)loc = json_real_value(val);
            break;
        case 'N' :
            *(int*)loc = (int) json_integer_value(val);
            break;
        case 'B' :
            *(char*)loc = (char) json_integer_value(val);
            break;
        case 'S' :
            *(int16_t*)loc = (int16_t) json_integer_value(val);
            break;
        case 'I' :
            *(int32_t*)loc = (int32_t) json_integer_value(val);
            break;
        case 'J' :
            *(int64_t*)loc = (int64_t) json_integer_value(val);
            break;
        case 'b' :
            *(uint8_t*)loc = (uint8_t) json_integer_value(val);
            break;
        case 's' :
            *(uint16_t*)loc = (uint16_t) json_integer_value(val);
            break;
        case 'i' :
            *(uint32_t*)loc = (uint32_t) json_integer_value(val);
            break;
        case 'j' :
            *(uint64_t*)loc = (uint64_t) json_integer_value(val);
            break;
        case 'E' :
            if (json_is_string(val)){
                status = jsonSerializer_parseEnum(type, json_string_value(val), loc);
            } else {
                status = ERROR;
                celix_err_pushf("Expected json string for enum type but got %i", json_typeof(val));
            }
            break;
        case 't' :
            if (json_is_null(val)) {
                // NULL string is allowed
            } else if (json_is_string(val)) {
                status = dynType_text_allocAndInit(type, loc, json_string_value(val));
            } else {
                status = ERROR;
                celix_err_pushf("Expected json string type got %i", json_typeof(val));
            }
            break;
        case '[' :
            if (json_is_array(val)) {
                status = jsonSerializer_parseSequence(type, val, loc);
            } else {
                status = ERROR;
                celix_err_pushf("Expected json array type got '%i'", json_typeof(val));
            }
            break;
        case '{' :
            if (json_is_object(val)) {
                status = jsonSerializer_parseObject(type, val, loc);
            } else {
                status = ERROR;
                celix_err_pushf("Expected json object type got '%i'", json_typeof(val));
            }
            break;
        case '*' :
            subType = dynType_typedPointer_getTypedType(type);
            if (dynType_ffiType(subType) != &ffi_type_pointer) {
                // NULL pointer is allowed
                if (!json_is_null(val)) {
                    status = jsonSerializer_createType(subType, val, (void **) loc);
                }
            } else {
                status = ERROR;
                celix_err_pushf("Error cannot deserialize pointer to pointer");
            }
            break;
        case 'l':
            status = jsonSerializer_parseAny(type->ref.ref, loc, val);
            break;
        case 'P' :
        default :
            status = ERROR;
            celix_err_pushf("Error provided type '%c' not supported for JSON\n", c);
            break;
    }

    return status;
}

static int jsonSerializer_parseSequence(const dyn_type* seq, json_t* array, void* seqLoc) {
    assert(dynType_type(seq) == DYN_TYPE_SEQUENCE);
    int status = OK;

    size_t size = json_array_size(array);
    if (size > UINT32_MAX) {
        celix_err_pushf("Error array size(%zu) too large", size);
        return ERROR;
    }
    if ((status = dynType_sequence_alloc(seq, seqLoc, (uint32_t) size)) != OK) {
        return status;
    }

    const dyn_type* itemType = dynType_sequence_itemType(seq);
    size_t index;
    json_t* val;
    json_array_foreach(array, index, val) {
        void* valLoc = NULL;
        (void)dynType_sequence_increaseLengthAndReturnLastLoc(seq, seqLoc, &valLoc);
        status = jsonSerializer_parseAny(itemType, valLoc, val);
        if (status != OK) {
            break;
        }
    }

    return status;
}

int jsonSerializer_serialize(const dyn_type* type, const void* input, char** output) {
    int status = OK;

    json_t* root = NULL;
    status = jsonSerializer_serializeJson(type, input, &root);

    if (status == OK) {
        *output = json_dumps(root, JSON_COMPACT | JSON_ENCODE_ANY);
        json_decref(root);
    }

    return status;
}

static int jsonSerializer_parseEnum(const dyn_type* type, const char* enum_name, int32_t* out) {
    struct meta_entry* entry;

    TAILQ_FOREACH(entry, &type->metaProperties, entries) {
        if (0 == strcmp(enum_name, entry->name)) {
            *out = atoi(entry->value);
            return OK;
        }
    }

    celix_err_pushf("Could not find Enum value %s in enum type", enum_name);
    return ERROR;
}

int jsonSerializer_serializeJson(const dyn_type* type, const void* input, json_t** out) {
    return jsonSerializer_writeAny(type, (void*)input /*TODO update static function to take const void**/, out);
}

static int jsonSerializer_writeAny(const dyn_type *type, void* input, json_t **out) {
    int status = OK;

    int descriptor = dynType_descriptorType(type);
    json_t *val = NULL;
    const dyn_type *subType = NULL;

    bool *z;            //Z
    float *f;           //F
    double *d;          //D
    char *b;            //B
    int *n;             //N
    int16_t *s;         //S
    int32_t *i;         //I
    int32_t *e;         //E
    int64_t *l;         //J
    uint8_t   *ub;      //b
    uint16_t  *us;      //s
    uint32_t  *ui;      //i
    uint64_t  *ul;      //j

    switch (descriptor) {
        case 'Z' :
            z = input;
            val = json_boolean((bool)*z);
            break;
        case 'B' :
            b = input;
            val = json_integer((json_int_t)*b);
            break;
        case 'S' :
            s = input;
            val = json_integer((json_int_t)*s);
            break;
        case 'I' :
            i = input;
            val = json_integer((json_int_t)*i);
            break;
        case 'J' :
            l = input;
            val = json_integer((json_int_t)*l);
            break;
        case 'b' :
            ub = input;
            val = json_integer((json_int_t)*ub);
            break;
        case 's' :
            us = input;
            val = json_integer((json_int_t)*us);
            break;
        case 'i' :
            ui = input;
            val = json_integer((json_int_t)*ui);
            break;
        case 'j' :
            ul = input;
            val = json_integer((json_int_t)*ul);
            break;
        case 'N' :
            n = input;
            val = json_integer((json_int_t)*n);
            break;
        case 'F' :
            f = input;
            val = json_real((double) *f);
            break;
        case 'D' :
            d = input;
            val = json_real(*d);
            break;
        case 't' :
            val = json_string(*(const char **) input);
            break;
        case 'E':
            e = input;
            status = jsonSerializer_writeEnum(type, *e, &val);
            break;
        case '*' :
            subType = dynType_typedPointer_getTypedType(type);
            status = jsonSerializer_writeAny(subType, *(void **)input, &val);
            break;
        case '{' :
            status = jsonSerializer_writeComplex(type, input, &val);
            break;
        case '[' :
            status = jsonSerializer_writeSequence(type, input, &val);
            break;
        case 'P' :
            status = ERROR;
            celix_err_pushf("Untyped pointer not supported for serialization.");
            break;
        case 'l':
            status = jsonSerializer_writeAny(type->ref.ref, input, out);
            break;
        default :
            celix_err_pushf("Unsupported descriptor '%c'", descriptor);
            status = ERROR;
            break;
    }

    if (status == OK && val != NULL) {
        *out = val;
    }

    return status;
}

static int jsonSerializer_writeSequence(const dyn_type *type, void *input, json_t **out) {
    assert(dynType_type(type) == DYN_TYPE_SEQUENCE);
    int status = OK;

    json_t *array = json_array();
    const dyn_type *itemType = dynType_sequence_itemType(type);
    uint32_t len = dynType_sequence_length(input);

    uint32_t i = 0;
    void *itemLoc = NULL;
    json_t *item = NULL;
    for (i = 0; i < len; i += 1) {
        item = NULL;
        status = dynType_sequence_locForIndex(type, input, i, &itemLoc);
        if (status == OK) {
            status = jsonSerializer_writeAny(itemType, itemLoc, &item);
            if (status == OK) {
                json_array_append(array, item);
                json_decref(item);
            }
        }

        if (status != OK) {
            break;
        }
    }

    if (status == OK && array != NULL) {
        *out = array;
    } else {
        *out = NULL;
        json_decref(array);
    }

    return status;
}

static int jsonSerializer_writeComplex(const dyn_type *type, void *input, json_t **out) {
    assert(dynType_type(type) == DYN_TYPE_COMPLEX);
    int status = OK;

    json_t *val = json_object();
    struct complex_type_entry *entry = NULL;
    const struct complex_type_entries_head *entries = dynType_complex_entries(type);
    int index = -1;

    TAILQ_FOREACH(entry, entries, entries) {
        void *subLoc = NULL;
        json_t *subVal = NULL;
        const dyn_type* subType = NULL;
        index = dynType_complex_indexForName(type, entry->name);
        if (index < 0) {
            celix_err_pushf("Cannot find index for member '%s'", entry->name);
            status = ERROR;
        }
        if(status == OK){
            subLoc = dynType_complex_valLocAt(type, index, input);
        }
        if (status == OK) {
            subType = dynType_complex_dynTypeAt(type, index);
        }
        if (status == OK) {
            status = jsonSerializer_writeAny(subType, subLoc, &subVal);
        }
        if (status == OK) {
            json_object_set(val, entry->name, subVal);
            json_decref(subVal);
        }

        if (status != OK) {
            break;
        }
    }

    if (status == OK && val != NULL) {
        *out = val;
    } else {
        *out = NULL;
        json_decref(val);
    }

    return status;
}

static int jsonSerializer_writeEnum(const dyn_type* type, int32_t enum_value, json_t **out) {
    struct meta_entry * entry;

    // Convert to string
    char enum_value_str[32];
    snprintf(enum_value_str, 32, "%d", enum_value);

    // Lookup in meta-information
    TAILQ_FOREACH(entry, &type->metaProperties, entries) {
        if (0 == strcmp(enum_value_str, entry->value)) {
            *out = json_string((const char*)entry->name);
            return OK;
        }
    }

    celix_err_pushf("Could not find Enum value %s in enum type", enum_value_str);
    return ERROR;
}
