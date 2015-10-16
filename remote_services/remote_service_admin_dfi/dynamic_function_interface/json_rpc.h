/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __JSON_RPC_H_
#define __JSON_RPC_H_

#include <jansson.h>
#include "dfi_log_util.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"

//logging
DFI_SETUP_LOG_HEADER(jsonRpc);

int jsonRpc_call(dyn_interface_type *intf, void *service, const char *request, char **out);


int jsonRpc_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out);
int jsonRpc_handleReply(dyn_function_type *func, const char *reply, void *args[]);

#endif
