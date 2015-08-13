/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __JSON_SERIALIZER_H_
#define __JSON_SERIALIZER_H_

#include <jansson.h>
#include "dfi_log_util.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"

//logging
DFI_SETUP_LOG_HEADER(jsonSerializer);

int jsonSerializer_deserialize(dyn_type *type, const char *input, void **result);
int jsonSerializer_deserializeJson(dyn_type *type, json_t *input, void **result);

int jsonSerializer_serialize(dyn_type *type, void *input, char **output);
int jsonSerializer_serializeJson(dyn_type *type, void *input, json_t **out);

int jsonSerializer_call(dyn_interface_type *intf, void *service, const char *request, char **out);

int jsonSerializer_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out);
int jsonSerializer_handleReply(dyn_function_type *func, const char *reply, void *args[]);
#endif
