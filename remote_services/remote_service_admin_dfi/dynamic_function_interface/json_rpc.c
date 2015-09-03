/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "json_rpc.h"
#include "json_serializer.h"
#include "dyn_type.h"
#include "dyn_interface.h"

#include <jansson.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>


static int OK = 0;
static int ERROR = 1;

DFI_SETUP_LOG(jsonRpc);

typedef void (*gen_func_type)(void);

struct generic_service_layout {
    void *handle;
    gen_func_type methods[];
};

int jsonRpc_call(dyn_interface_type *intf, void *service, const char *request, char **out) {
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
        dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
        const char *argMeta = dynType_getMetaInfo(argType, "am");
        if (argMeta == NULL) {
            printf("setting std for %i\n", i);
            value = json_array_get(arguments, index++);
            status = jsonSerializer_deserializeJson(argType, value, &(args[i]));
        } else if (strcmp(argMeta, "pre") == 0) {
            printf("setting pre alloc output for %i\n", i);
            dynType_alloc(argType, &args[i]);

        } else if ( strcmp(argMeta, "out") == 0) {
            printf("setting output for %i\n", i);
            args[i] = NULL;
        } else if (strcmp(argMeta, "handle") == 0) {
            printf("setting handle for %i\n", i);
            args[i] = &handle;
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
        dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
        const char *argMeta = dynType_getMetaInfo(argType, "am");
        if (argMeta == NULL) {
            //ignore
        } else if (strcmp(argMeta, "pre") == 0)  {
            if (status == OK) {
                status = jsonSerializer_serializeJson(argType, args[i], &jsonResult);
            }
        } else if (strcmp(argMeta, "out") == 0) {
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

int jsonRpc_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out) {
    int status = OK;


    LOG_DEBUG("Calling remote function '%s'\n", id);
    json_t *invoke = json_object();
    json_object_set(invoke, "m", json_string(id));

    json_t *arguments = json_array();
    json_object_set_new(invoke, "a", arguments);

    int i;
    int nrOfArgs = dynFunction_nrOfArguments(func);
    for (i = 0; i < nrOfArgs; i +=1) {
        dyn_type *type = dynFunction_argumentTypeForIndex(func, i);
        const char *argMeta = dynType_getMetaInfo(type, "am");
        if (argMeta == NULL) {
            json_t *val = NULL;

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

int jsonRpc_handleReply(dyn_function_type *func, const char *reply, void *args[]) {
    int status = 0;

    json_t *replyJson = json_loads(reply, JSON_DECODE_ANY, NULL); //TODO check
    json_t *result = json_object_get(replyJson, "r"); //TODO check

    LOG_DEBUG("replyJson ptr is %p and result ptr is %p\n", replyJson, result);

    int nrOfArgs = dynFunction_nrOfArguments(func);
    int i;
    for (i = 0; i < nrOfArgs; i += 1) {
        dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
        const char *argMeta = dynType_getMetaInfo(argType, "am");
        if (argMeta == NULL) {
            //skip
        } else if (strcmp(argMeta, "pre") == 0) {
            LOG_DEBUG("found pre argument at %i", i);
            dyn_type *subType = NULL;
            dynType_typedPointer_getTypedType(argType, &subType);
            void *tmp = NULL;
            size_t size = dynType_size(subType);
            status = jsonSerializer_deserializeJson(subType, result, &tmp);
            void **out = (void **)args[i];
            memcpy(*out, tmp, size);
            dynType_free(subType, tmp);
        } else if (strcmp(argMeta, "out") == 0) {
            assert(false); //TODO
        }
    }

    json_decref(replyJson);

    return status;
}