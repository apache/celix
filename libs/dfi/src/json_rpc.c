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

#include "json_rpc.h"
#include "json_serializer.h"
#include "dyn_type.h"
#include "dyn_interface.h"
#include <jansson.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <ffi.h>
#include "celix_compiler.h"
#include "dyn_type_common.h"

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

	dyn_type* returnType = NULL;

	json_error_t error;
	json_t *js_request = json_loads(request, 0, &error);
	json_t *arguments = NULL;
	const char *sig;
	if (js_request) {
		if (json_unpack(js_request, "{s:s}", "m", &sig) != 0) {
            LOG_ERROR("Got json error '%s'\n", error.text);
            json_decref(js_request);
            return ERROR;
		} else {
			arguments = json_object_get(js_request, "a");
		}
	} else {
        LOG_ERROR("Got json error '%s' for '%s'\n", error.text, request);
		return ERROR;
	}

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
	}
	else if (status == OK) {
		returnType = dynFunction_returnType(method->dynFunc);
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

	void *ptr = NULL;
	void *ptrToPtr = &ptr;

	//setup and deserialize input
	for (i = 0; i < nrOfArgs; ++i) {
		dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
		enum dyn_function_argument_meta  meta = dynFunction_argumentMetaForIndex(func, i);
		if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
			value = json_array_get(arguments, index++);
			void *outPtr = NULL;
            status = jsonSerializer_deserializeJson(argType, value, &outPtr);
            args[i] = outPtr;
		} else if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
		    void **instPtr = calloc(1, sizeof(void*));
		    void *inst = NULL;
		    dyn_type *subType = NULL;
		    dynType_typedPointer_getTypedType(argType, &subType);
            dynType_alloc(subType, &inst);
            *instPtr = inst;
            args[i] = instPtr;
		} else if (meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
			args[i] = &ptrToPtr;
		} else if (meta == DYN_FUNCTION_ARGUMENT_META__HANDLE) {
			args[i] = &handle;
		}

		if (status != OK) {
			break;
		}
	}
	json_decref(js_request);

	if (status == OK) {
		if (dynType_descriptorType(returnType) != 'N') {
			//NOTE To be able to handle exception only N as returnType is supported
			LOG_ERROR("Only interface methods with a native int are supported. Found type '%c'", (char)dynType_descriptorType(returnType));
			status = ERROR;
		}
	}

	ffi_sarg returnVal = 1;

	if (status == OK) {
		status = dynFunction_call(func, fp, (void *) &returnVal, args);
	}

	int funcCallStatus = (int)returnVal;

    //free input args
	json_t *jsonResult = NULL;
	for(i = 0; i < nrOfArgs; ++i) {
		dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
		enum dyn_function_argument_meta meta = dynFunction_argumentMetaForIndex(func, i);
		if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
		    if (dynType_descriptorType(argType) == 't') {
		        const char* isConst = dynType_getMetaInfo(argType, "const");
		        if (isConst != NULL && strncmp("true", isConst, 5) == 0) {
		            dynType_free(argType, args[i]);
		        } else {
                    //char* -> callee is now owner, no free for char seq needed
                    //will free the actual pointer
                    free(args[i]);
		        }
		    } else {
                dynType_free(argType, args[i]);
            }
		}
	}

	//serialize and free output
	for (i = 0; i < nrOfArgs; i += 1) {
		dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
		enum dyn_function_argument_meta  meta = dynFunction_argumentMetaForIndex(func, i);
		if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
			if (funcCallStatus == 0 && status == OK) {
				status = jsonSerializer_serializeJson(argType, args[i], &jsonResult);
			}
			dyn_type *subType = NULL;
			dynType_typedPointer_getTypedType(argType, &subType);
			void **ptrToInst = (void**)args[i];
			dynType_free(subType, *ptrToInst);
			free(ptrToInst);
		} else if (meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
			if (funcCallStatus == 0 && ptr != NULL) {
				dyn_type *typedType = NULL;
				if (status == OK) {
					status = dynType_typedPointer_getTypedType(argType, &typedType);
				}
				if (status == OK && dynType_descriptorType(typedType) == 't') {
					status = jsonSerializer_serializeJson(typedType, (void*) &ptr, &jsonResult);
					free(ptr);
				} else {
					dyn_type *typedTypedType = NULL;
					if (status == OK) {
						status = dynType_typedPointer_getTypedType(typedType, &typedTypedType);
					}

					if(status == OK){
						status = jsonSerializer_serializeJson(typedTypedType, ptr, &jsonResult);
					}

					if (status == OK) {
						dynType_free(typedTypedType, ptr);
					}
				}

			}
		}

		if (status != OK) {
			break;
		}
	}

	char *response = NULL;
	if (status == OK) {
		json_t *payload = json_object();
		if (funcCallStatus == 0) {
			if (jsonResult == NULL) {
				//ignore -> no result
			} else {
                json_object_set_new_nocheck(payload, "r", jsonResult);
			}
		} else {
            json_object_set_new_nocheck(payload, "e", json_integer(funcCallStatus));
		}
		response = json_dumps(payload, JSON_COMPACT | JSON_ENCODE_ANY);//Should use JSON_COMPACT, it can reduce the size of the JSON string.
		json_decref(payload);
	}

	if (status == OK) {
		*out = response;
	} else {
		free(response);
	}

	return status;
}

int jsonRpc_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out) {
	int status = OK;


	json_t *invoke = json_object();
    json_object_set_new_nocheck(invoke, "m", json_string(id));

	json_t *arguments = json_array();
    json_object_set_new_nocheck(invoke, "a", arguments);

	int i;
	int nrOfArgs = dynFunction_nrOfArguments(func);
	for (i = 0; i < nrOfArgs; i +=1) {
		dyn_type *type = dynFunction_argumentTypeForIndex(func, i);
		enum dyn_function_argument_meta  meta = dynFunction_argumentMetaForIndex(func, i);
		if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
			json_t *val = NULL;

			int rc = jsonSerializer_serializeJson(type, args[i], &val);

            if (dynType_descriptorType(type) == 't') {
                const char *metaArgument = dynType_getMetaInfo(type, "const");
                if (metaArgument != NULL && strncmp("true", metaArgument, 5) == 0) {
                    //const char * as input -> nop
                } else {
                    char **str = args[i];
                    free(*str); //char * as input -> got ownership -> free it.
                }
            }

			if (rc == 0) {
				json_array_append_new(arguments, val);
			} else {
                LOG_ERROR("Failed to serialize args for function '%s'\n", id);
				status = ERROR;
				break;
			}
		} else {
			//skip handle / output types
		}
	}

	char *invokeStr = json_dumps(invoke, JSON_COMPACT | JSON_ENCODE_ANY);//Should use JSON_COMPACT, it can reduce the size of the JSON string.
	json_decref(invoke);

	if (status == OK) {
		*out = invokeStr;
	} else {
        *out = NULL;
        free(invokeStr);
    }

	return status;
}

int jsonRpc_handleReply(dyn_function_type *func, const char *reply, void *args[], int *rsErrno) {
	int status = OK;

	json_error_t error;
	json_t *replyJson = json_loads(reply, JSON_DECODE_ANY, &error);
	if (replyJson == NULL) {
		status = ERROR;
		LOG_ERROR("Error parsing json '%s', got error '%s'", reply, error.text);
	}

	json_t *result = NULL;
	json_t *rsError = NULL;
    bool replyHasError = false;
	if (status == OK) {
		*rsErrno = 0;
		result = json_object_get(replyJson, "r");
		if (result == NULL) {
			rsError = json_object_get(replyJson, "e");
			if(rsError != NULL) {
				//get the invocation error of remote service function
				*rsErrno = (int)json_integer_value(rsError);
                replyHasError = true;
			}
		}
	}

    int nrOfOutputArgs = 0;
    int nrOfArgs = dynFunction_nrOfArguments(func);
    for (int j = 0; j < nrOfArgs; ++j) {
        enum dyn_function_argument_meta meta = dynFunction_argumentMetaForIndex(func, j);
        if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT || meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
            nrOfOutputArgs += 1;
            if (nrOfOutputArgs > 1) {
                status = ERROR;
                LOG_ERROR("Only one output argument is supported");
                break;
            }
            if (result == NULL && !replyHasError) {
                status = ERROR;
                LOG_ERROR("Expected result in reply. got '%s'", reply);
                break;
            }
        }
    }

	if (status == OK && !replyHasError) {
        int i;
		for (i = 0; i < nrOfArgs; i += 1) {
			dyn_type *argType = dynFunction_argumentTypeForIndex(func, i);
			enum dyn_function_argument_meta meta = dynFunction_argumentMetaForIndex(func, i);
			if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
				void *tmp = NULL;
				void **out = (void **) args[i];
				size_t size = 0;

                if (dynType_descriptorType(argType) == 't') {
					status = jsonSerializer_deserializeJson(argType, result, &tmp);
					if (tmp != NULL) {
						size = strnlen(((char *) *(char**) tmp), 1024 * 1024);
						memcpy(*out, *(void**) tmp, size);
					}
				} else {
					dynType_typedPointer_getTypedType(argType, &argType);
					status = jsonSerializer_deserializeJson(argType, result, &tmp);
					if (tmp != NULL) {
						size = dynType_size(argType);
						memcpy(*out, tmp, size);
					}
				}

				dynType_free(argType, tmp);
			} else if (meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
				dyn_type *subType = NULL;

				dynType_typedPointer_getTypedType(argType, &subType);

                if (dynType_descriptorType(subType) == 't') {
				    char ***out = (char ***) args[i];
                    char **ptrToString = NULL;
                    status = jsonSerializer_deserializeJson(subType, result, (void**)&ptrToString);
                    char *s CELIX_UNUSED = *ptrToString; //note for debug
                    free(ptrToString);
                    **out = (void*)s;
                } else {
					dyn_type *subSubType = NULL;
					dynType_typedPointer_getTypedType(subType, &subSubType);
					void ***out = (void ***) args[i];
					status = jsonSerializer_deserializeJson(subSubType, result, *out);
				}
			} else {
				//skip
			}
		}
	}


	json_decref(replyJson);

	return status;
}
