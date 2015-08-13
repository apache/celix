/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "json_serializer.h"
#include "dyn_type.h"
#include "dyn_interface.h"

#include <jansson.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

static int jsonSerializer_createType(dyn_type *type, json_t *object, void **result);
static int jsonSerializer_parseObject(dyn_type *type, json_t *object, void *inst);
static int jsonSerializer_parseObjectMember(dyn_type *type, const char *name, json_t *val, void *inst);
static int jsonSerializer_parseSequence(dyn_type *seq, json_t *array, void *seqLoc);
static int jsonSerializer_parseAny(dyn_type *type, void *input, json_t *val);

static int jsonSerializer_writeAny(dyn_type *type, void *input, json_t **val);

static int jsonSerializer_writeComplex(dyn_type *type, void *input, json_t **val);

static int jsonSerializer_writeSequence(dyn_type *type, void *input, json_t **out);

static int OK = 0;
static int ERROR = 1;

DFI_SETUP_LOG(jsonSerializer);

typedef void (*gen_func_type)(void);

struct generic_service_layout {
    void *handle;
    gen_func_type methods[];
};

int jsonSerializer_deserialize(dyn_type *type, const char *input, void **result) {
    assert(dynType_type(type) == DYN_TYPE_COMPLEX);
    int status = 0;

    json_error_t error;
    json_t *root = json_loads(input, JSON_DECODE_ANY, &error);

    if (root != NULL) {
        status = jsonSerializer_deserializeJson(type, root, result);
        json_decref(root);
    } else {
        status = ERROR;
        LOG_ERROR("Error parsing json input '%s'. Error is %s\n", input, error.text);
    }

    return status;
}

int jsonSerializer_deserializeJson(dyn_type *type, json_t *input, void **out) {
    return jsonSerializer_createType(type, input, out);
}

static int jsonSerializer_createType(dyn_type *type, json_t *val, void **result) {
    assert(val != NULL);
    int status = OK;

    void *inst = NULL;
    status = dynType_alloc(type, &inst);

    if (status == OK) {
        assert(inst != NULL);
        status = jsonSerializer_parseAny(type, inst, val);

        if (status != OK) {
            dynType_free(type, inst);
        }
    }

    if (status == OK) {
        *result = inst;
    }

    return status;
}

static int jsonSerializer_parseObject(dyn_type *type, json_t *object, void *inst) {
    assert(object != NULL);
    int status = 0;
    json_t *value;
    const char *key;

    json_object_foreach(object, key, value) {
        status = jsonSerializer_parseObjectMember(type, key, value, inst);
        if (status != OK) {
            break;
        }
    }

    return status;
}

static int jsonSerializer_parseObjectMember(dyn_type *type, const char *name, json_t *val, void *inst) {
    int status = OK;
    int index = dynType_complex_indexForName(type, name);
    void *valp = NULL;
    dyn_type *valType = NULL;

    status = dynType_complex_valLocAt(type, index, inst, &valp);

    if (status == OK ) {
        status = dynType_complex_dynTypeAt(type, index, &valType);
    }

    if (status == OK) {
        status = jsonSerializer_parseAny(valType, valp, val);
    }

    return status;
}

static int jsonSerializer_parseAny(dyn_type *type, void *loc, json_t *val) {
    int status = OK;

    dyn_type *subType = NULL;
    char c = dynType_descriptorType(type);

    float *f;           //F
    double *d;          //D
    char *b;            //B
    int *n;             //N
    int16_t *s;         //S
    int32_t *i;         //I
    int64_t *l;         //J
    uint16_t  *us;      //s
    uint32_t  *ui;      //i
    uint64_t  *ul;      //j

    switch (c) {
        case 'F' :
            f = loc;
            *f = (float) json_real_value(val);
            break;
        case 'D' :
            d = loc;
            *d = json_real_value(val);
            break;
        case 'N' :
            n = loc;
            *n = (int) json_real_value(val);
            break;
        case 'B' :
            b = loc;
            *b = (char) json_integer_value(val);
            break;
        case 'S' :
            s = loc;
            *s = (int16_t) json_integer_value(val);
            break;
        case 'I' :
            i = loc;
            *i = (int32_t) json_integer_value(val);
            break;
        case 'J' :
            l = loc;
            *l = (int64_t) json_integer_value(val);
            break;
        case 's' :
            us = loc;
            *us = (uint16_t) json_integer_value(val);
            break;
        case 'i' :
            ui = loc;
            *ui = (uint32_t) json_integer_value(val);
            break;
        case 'j' :
            ul = loc;
            *ul = (uint64_t) json_integer_value(val);
            break;
        case 't' :
            if (json_is_string(val)) {
                dynType_text_allocAndInit(type, loc, json_string_value(val));
            } else {
                status = ERROR;
                LOG_ERROR("Expected json string type got %i", json_typeof(val));
            }
            break;
        case '[' :
            if (json_is_array(val)) {
                status = jsonSerializer_parseSequence(type, val, loc);
            } else {
                status = ERROR;
                LOG_ERROR("Expected json array type got '%i'", json_typeof(val));
            }
            break;
        case '{' :
            if (status == OK) {
                status = jsonSerializer_parseObject(type, val, loc);
            }
            break;
        case '*' :
            status = dynType_typedPointer_getTypedType(type, &subType);
            if (status == OK) {
                status = jsonSerializer_createType(subType, val, (void **) loc);
            }
            break;
        case 'P' :
            status = ERROR;
            LOG_WARNING("Untyped pointer are not supported for serialization");
            break;
        default :
            status = ERROR;
            LOG_ERROR("Error provided type '%c' not supported for JSON\n", dynType_descriptorType(type));
            break;
    }

    return status;
}

static int jsonSerializer_parseSequence(dyn_type *seq, json_t *array, void *seqLoc) {
    assert(dynType_type(seq) == DYN_TYPE_SEQUENCE);
    int status = OK;

    size_t size = json_array_size(array);
    LOG_DEBUG("Allocating sequence with capacity %zu", size);
    status = dynType_sequence_alloc(seq, seqLoc, (int) size);

    if (status == OK) {
        dyn_type *itemType = dynType_sequence_itemType(seq);
        size_t index;
        json_t *val;
        json_array_foreach(array, index, val) {
            void *valLoc = NULL;
            status = dynType_sequence_increaseLengthAndReturnLastLoc(seq, seqLoc, &valLoc);
            LOG_DEBUG("Got sequence loc %p", valLoc);

            if (status == OK) {
                status = jsonSerializer_parseAny(itemType, valLoc, val);
                if (status != OK) {
                    break;
                }
            }
        }
    }

    return status;
}

int jsonSerializer_serialize(dyn_type *type, void *input, char **output) {
    int status = OK;

    json_t *root = NULL;
    status = jsonSerializer_serializeJson(type, input, &root);

    if (status == OK) {
        *output = json_dumps(root, JSON_COMPACT);
        json_decref(root);
    }

    return status;
}

int jsonSerializer_serializeJson(dyn_type *type, void *input, json_t **out) {
    return jsonSerializer_writeAny(type, input, out);
}

static int jsonSerializer_writeAny(dyn_type *type, void *input, json_t **out) {
    int status = OK;

    int descriptor = dynType_descriptorType(type);
    json_t *val = NULL;
    dyn_type *subType = NULL;

    float *f;           //F
    double *d;          //D
    char *b;            //B
    int *n;             //N
    int16_t *s;         //S
    int32_t *i;         //I
    int64_t *l;         //J
    uint16_t  *us;      //s
    uint32_t  *ui;      //i
    uint64_t  *ul;      //j

    switch (descriptor) {
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
        case '*' :
            status = dynType_typedPointer_getTypedType(type, &subType);
            if (status == OK) {
                status = jsonSerializer_writeAny(subType, *(void **)input, &val);
            }
            break;
        case '{' :
            status = jsonSerializer_writeComplex(type, input, &val);
            break;
        case '[' :
            status = jsonSerializer_writeSequence(type, input, &val);
            break;
        case 'P' :
            LOG_WARNING("Untyped pointer not supported for serialization. ignoring");
            break;
        default :
            LOG_ERROR("Unsupported descriptor '%c'", descriptor);
            status = ERROR;
            break;
    }

    if (status == OK && val != NULL) {
        *out = val;
    }

    return status;
}

static int jsonSerializer_writeSequence(dyn_type *type, void *input, json_t **out) {
    assert(dynType_type(type) == DYN_TYPE_SEQUENCE);
    int status = OK;

    json_t *array = json_array();
    dyn_type *itemType = dynType_sequence_itemType(type);
    uint32_t len = dynType_sequence_length(input);

    int i = 0;
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
    }

    return status;
}

static int jsonSerializer_writeComplex(dyn_type *type, void *input, json_t **out) {
    assert(dynType_type(type) == DYN_TYPE_COMPLEX);
    int status = OK;

    json_t *val = json_object();
    struct complex_type_entry *entry = NULL;
    struct complex_type_entries_head *entries = NULL;
    int index = -1;

    status = dynType_complex_entries(type, &entries);
    if (status == OK) {
        TAILQ_FOREACH(entry, entries, entries) {
            void *subLoc = NULL;
            json_t *subVal = NULL;
            dyn_type *subType = NULL;
            index = dynType_complex_indexForName(type, entry->name);
            status = dynType_complex_valLocAt(type, index, input, &subLoc);
            if (status == OK ) {
                status = dynType_complex_dynTypeAt(type, index, &subType);
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
    }

    if (status == OK && val != NULL) {
        *out = val;
    }

    return status;
}

int jsonSerializer_call(dyn_interface_type *intf, void *service, const char *request, char **out) {
    int status = OK;

    LOG_DEBUG("Parsing data: %s\n", request);
    json_error_t error;
    json_t *js_request = json_loads(request, 0, &error);
    json_t *arguments = NULL;
    const char *sig;
    if (js_request) {
        if (json_unpack(js_request, "{s:s}", "m", &sig) != 0) {
            LOG_ERROR("Got json error '%s'\n", error.text);
        } else {
            arguments = json_object_get(js_request, "a");
        }
    } else {
        LOG_ERROR("Got json error '%s' for '%s'\n", error.text, request);
        return 0;
    }

    LOG_DEBUG("Looking for method %s\n", sig);
    struct methods_head *methods = NULL;
    dynInterface_methods(intf, &methods);
    struct method_entry *entry = NULL;
    struct method_entry *method = NULL;
    TAILQ_FOREACH(entry, methods, entries) {
        if (strcmp(sig, entry->id) == 0) {
            method = entry;
            break;
        }
    }

    if (method == NULL) {
        status = ERROR;
        LOG_ERROR("Cannot find method with sig '%s'", sig);
    } else {
        LOG_DEBUG("RSA: found method '%s'\n", entry->id);
    }

    void (*fp)(void) = NULL;
    void *handle = NULL;
    if (status == OK) {
        struct generic_service_layout *serv = service;
        handle = serv->handle;
        fp = serv->methods[method->index];
    }

    dyn_function_type *func = NULL;
    int nrOfArgs = 0;
    if (status == OK) {
        nrOfArgs = dynFunction_nrOfArguments(entry->dynFunc);
        func = entry->dynFunc;
    }

    void *args[nrOfArgs];

    json_t *value = NULL;

    int i;
    int index = 0;
    for (i = 0; i < nrOfArgs; i += 1) {
        int metaInfo = dynFunction_argumentMetaInfoForIndex(func, i);
        dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
        if (metaInfo == DYN_FUNCTION_ARG_META_PRE_ALLOCATED_OUTPUT_TYPE) {
            printf("setting pre alloc output for %i\n", i);
            dynType_alloc(argType, &args[i]);

        } else if ( metaInfo == DYN_FUNCTION_ARG_META_OUTPUT_TYPE) {
            printf("setting output for %i\n", i);
            args[i] = NULL;
        } else if (metaInfo == DYN_FUNCTION_ARG_META_HANDLE_TYPE) {
            printf("setting handle for %i\n", i);
            args[i] = &handle;
        } else {
            printf("setting std for %i\n", i);
            value = json_array_get(arguments, index++);
            status = jsonSerializer_deserializeJson(argType, value, &(args[i]));
        }

        if (status != OK) {
            break;
        }
    }
    json_decref(js_request);


    //TODO assert return type is native int
    int returnVal = 0;
    dynFunction_call(func, fp, (void *)&returnVal, args);
    printf("done calling\n");
    double **r = args[2];
    printf("result ptrptr is %p, result ptr %p, result is %f\n", r, *r, **r);


    json_t *jsonResult = NULL;
    for (i = 0; i < nrOfArgs; i += 1) {
        int metaInfo = dynFunction_argumentMetaInfoForIndex(func, i);
        dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
        if (metaInfo == DYN_FUNCTION_ARG_META_PRE_ALLOCATED_OUTPUT_TYPE) {
            if (status == OK) {
                status = jsonSerializer_serializeJson(argType, args[i], &jsonResult);
            }
        } else if (metaInfo == DYN_FUNCTION_ARG_META_OUTPUT_TYPE) {
            printf("TODO\n");
            assert(false);
        }

        if (status != OK) {
            break;
        }
    }

    char *response = NULL;
    if (status == OK) {
        LOG_DEBUG("creating payload\n");
        json_t *payload = json_object();
        json_object_set_new(payload, "r", jsonResult);
        response = json_dumps(payload, JSON_DECODE_ANY);
        json_decref(payload);
        LOG_DEBUG("status ptr is %p. response if '%s'\n", status, response);
    }

    if (status == OK) {
        *out = response;
    } else {
        if (response != NULL) {
            free(response);
        }
    }

    //TODO free args (created by jsonSerializer and dynType_alloc) (dynType_free)
    return status;
}

int jsonSerializer_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out) {
    int status = OK;


    LOG_DEBUG("Calling remote function '%s'\n", id);
    json_t *invoke = json_object();
    json_object_set(invoke, "m", json_string(id));

    json_t *arguments = json_array();
    json_object_set_new(invoke, "a", arguments);

    int i;
    int nrOfArgs = dynFunction_nrOfArguments(func);
    for (i = 0; i < nrOfArgs; i +=1) {
        if (dynFunction_argumentMetaInfoForIndex(func, i) == DYN_FUNCTION_ARG_META_INPUT_TYPE) {
            json_t *val = NULL;
            dyn_type *type = dynFunction_argumentTypeForIndex(func, i);
            int rc = jsonSerializer_serializeJson(type, args[i], &val);
            if (rc == 0) {
                json_array_append_new(arguments, val);
            } else {
                status = ERROR;
                break;
            }
        } else {
            //skip handle / output types
        }
    }

    char *invokeStr = json_dumps(invoke, JSON_DECODE_ANY);
    json_decref(invoke);

    if (status == OK) {
        *out = invokeStr;
    }

    return status;
}

int jsonSerializer_handleReply(dyn_function_type *func, const char *reply, void *args[]) {
    int status = 0;

    json_t *replyJson = json_loads(reply, JSON_DECODE_ANY, NULL); //TODO check
    json_t *result = json_object_get(replyJson, "r"); //TODO check

    LOG_DEBUG("replyJson ptr is %p and result ptr is %p\n", replyJson, result);

    int nrOfArgs = dynFunction_nrOfArguments(func);
    int i;
    for (i = 0; i < nrOfArgs; i += 1) {
        dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
        int metaInf = dynFunction_argumentMetaInfoForIndex(func, i);
        if (metaInf == DYN_FUNCTION_ARG_META_PRE_ALLOCATED_OUTPUT_TYPE) {
            dyn_type *subType = NULL;
            dynType_typedPointer_getTypedType(argType, &subType);
            void *tmp = NULL;
            size_t size = dynType_size(subType);
            status = jsonSerializer_deserializeJson(subType, result, &tmp);
            void **out = (void **)args[i];
            memcpy(*out, tmp, size);
            dynType_free(subType, tmp);
        } else if (metaInf == DYN_FUNCTION_ARG_META_OUTPUT_TYPE) {
            assert(false); //TODO
        } else {
            //skip handle and input types
        }
    }

    json_decref(replyJson);

    return status;
}