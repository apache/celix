/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include "etcd.h"

#define ETCD_JSON_NODE                  "node"
#define ETCD_JSON_PREVNODE              "prevNode"
#define ETCD_JSON_NODES                 "nodes"
#define ETCD_JSON_ACTION                "action"
#define ETCD_JSON_KEY                   "key"
#define ETCD_JSON_VALUE                 "value"
#define ETCD_JSON_DIR                   "dir"
#define ETCD_JSON_MODIFIEDINDEX         "modifiedIndex"

#define MAX_OVERHEAD_LENGTH           64
#define DEFAULT_CURL_TIMEOUT          10
#define DEFAULT_CURL_CONECTTIMEOUT    10

typedef enum {
	GET, PUT, DELETE
} request_t;

static const char* etcd_server;
static int etcd_port = 0;

struct MemoryStruct {
	char *memory;
	size_t size;
};


/**
 * Static function declarations
 */
static int performRequest(char* url, request_t request, void* callback, void* reqData, void* repData);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
/**
 * External function definition
 */


/**
 * etcd_init
 */
int etcd_init(const char* server, int port) {
        etcd_server = server;
        etcd_port = port;

        return curl_global_init(CURL_GLOBAL_ALL) != 0;
}


/**
 * etcd_get
 */
int etcd_get(const char* key, char** value, int* modifiedIndex) {
    json_t* js_root = NULL;
    json_t* js_node = NULL;
    json_t* js_value = NULL;
    json_t* js_modifiedIndex = NULL;
    json_error_t error;
    int res = -1;
    struct MemoryStruct reply;

    reply.memory = malloc(1); /* will be grown as needed by the realloc above */
    reply.size = 0; /* no data at this point */

    int retVal = -1;
    char *url;
    asprintf(&url, "http://%s:%d/v2/keys/%s", etcd_server, etcd_port, key);
    res = performRequest(url, GET, WriteMemoryCallback, NULL, (void*) &reply);
    free(url);
    if (res == CURLE_OK) {
        js_root = json_loads(reply.memory, 0, &error);

        if (js_root != NULL) {
            js_node = json_object_get(js_root, ETCD_JSON_NODE);
        }
        if (js_node != NULL) {
            js_value = json_object_get(js_node, ETCD_JSON_VALUE);
            js_modifiedIndex = json_object_get(js_node,
            ETCD_JSON_MODIFIEDINDEX);

            if (js_modifiedIndex != NULL && js_value != NULL) {
                if (modifiedIndex) {
                    *modifiedIndex = json_integer_value(js_modifiedIndex);
                }
                *value = strdup(json_string_value(js_value));
                retVal = 0;
            }
        }
        if (js_root != NULL) {
            json_decref(js_root);
        }
    }

    if (reply.memory) {
        free(reply.memory);
    }
    if(retVal != 0) {
        value = NULL;
    }
    return retVal;
}


static int etcd_get_recursive_values(json_t* js_root, etcd_key_value_callback callback, void *arg, json_int_t *mod_index) {
    json_t *js_nodes;
    if ((js_nodes = json_object_get(js_root, ETCD_JSON_NODES)) != NULL) {
        // subarray
        if (json_is_array(js_nodes)) {
            int len = json_array_size(js_nodes);
            for (int i = 0; i < len; i++) {
                json_t *js_object = json_array_get(js_nodes, i);
                json_t *js_mod_index = json_object_get(js_object, ETCD_JSON_MODIFIEDINDEX);

                if(js_mod_index != NULL) {
                    json_int_t index = json_integer_value(js_mod_index);
                    if(*mod_index < index) {
                        *mod_index = index;
                    }
                } else {
                    printf("[ETCDLIB] Error: No INDEX found for key!\n");
                }

                if (json_object_get(js_object, ETCD_JSON_NODES)) {
                    // node contains nodes
                    etcd_get_recursive_values(js_object, callback, arg, mod_index);
                } else {
                    json_t* js_key = json_object_get(js_object, ETCD_JSON_KEY);
                    json_t* js_value = json_object_get(js_object, ETCD_JSON_VALUE);

                    if (js_key && js_value) {
                        if (!json_object_get(js_object, ETCD_JSON_DIR)) {
                            callback(json_string_value(js_key), json_string_value(js_value), arg);
                        }
                    } //else empty etcd directory, not an error.

                }
            }
        } else {
            printf("[ETCDLIB] Error: misformatted JSON: nodes element is not an array !!\n");
        }
    } else {
        printf("[ETCDLIB] Error: nodes element not found!!\n");
    }

    return (*index > 0 ? 0 : 1);
}

/**
 * etcd_get_directory
 */
int etcd_get_directory(const char* directory, etcd_key_value_callback callback, void* arg, long long* modifiedIndex) {
    json_t* js_root = NULL;
    json_t* js_rootnode = NULL;

    json_error_t error;
    int res;
    struct MemoryStruct reply;

    reply.memory = malloc(1); /* will be grown as needed by the realloc above */
    reply.size = 0; /* no data at this point */

    int retVal = 0;
    char *url;

    asprintf(&url, "http://%s:%d/v2/keys/%s?recursive=true", etcd_server, etcd_port, directory);

    res = performRequest(url, GET, WriteMemoryCallback, NULL, (void*) &reply);
    free(url);

    if (res == CURLE_OK) {
        js_root = json_loads(reply.memory, 0, &error);
        if (js_root != NULL) {
            js_rootnode = json_object_get(js_root, ETCD_JSON_NODE);
        } else {
            retVal = -1;
            printf("ERROR ETCD: %s in js_root not found", ETCD_JSON_NODE);
        }
        if (js_rootnode != NULL) {
            *modifiedIndex = 0;
            retVal = etcd_get_recursive_values(js_rootnode, callback, arg, (json_int_t*)modifiedIndex);
        }
        if (js_root != NULL) {
            json_decref(js_root);
        }
    }

    if (reply.memory) {
        free(reply.memory);
    }

    return retVal;
}

/**
 * etcd_set
 */
int etcd_set(const char* key, const char* value, int ttl, bool prevExist) {
    json_error_t error;
    json_t* js_root = NULL;
    json_t* js_node = NULL;
    json_t* js_value = NULL;
    int retVal = -1;
    char *url;
    size_t req_len = strlen(value) + MAX_OVERHEAD_LENGTH;
    char request[req_len];
    char* requestPtr = request;
    int res;
    struct MemoryStruct reply;

    /* Skip leading '/', etcd cannot handle this. */
    while(*key == '/') {
        key++;
    }

    reply.memory = calloc(1, 1); /* will be grown as needed by the realloc above */
    reply.size = 0; /* no data at this point */

    asprintf(&url, "http://%s:%d/v2/keys/%s", etcd_server, etcd_port, key);

    requestPtr += snprintf(requestPtr, req_len, "value=%s", value);
    if (ttl > 0) {
        requestPtr += snprintf(requestPtr, req_len-(requestPtr-request), ";ttl=%d", ttl);
    }

    if (prevExist) {
        requestPtr += snprintf(requestPtr, req_len-(requestPtr-request), ";prevExist=true");
    }
    res = performRequest(url, PUT, WriteMemoryCallback, request, (void*) &reply);
    if(url) {
        free(url);
    }

    if (res == CURLE_OK) {
        js_root = json_loads(reply.memory, 0, &error);

        if (js_root != NULL) {
            js_node = json_object_get(js_root, ETCD_JSON_NODE);
        }
        if (js_node != NULL) {
            js_value = json_object_get(js_node, ETCD_JSON_VALUE);
        }
        if (js_value != NULL && json_is_string(js_value)) {
            if(strcmp(json_string_value(js_value), value) == 0) {
                retVal = 0;
            }
        }
        if (js_root != NULL) {
            json_decref(js_root);
        }
    }

    if (reply.memory) {
        free(reply.memory);
    }

    return retVal;
}


/**
 * etcd_set_with_check
 */
int etcd_set_with_check(const char* key, const char* value, int ttl, bool always_write) {
    char *etcd_value;
    int result = 0;
    if (etcd_get(key, &etcd_value, NULL) == 0) {
        if (strcmp(etcd_value, value) != 0) {
            printf("[ETCDLIB} WARNING: value already exists and is different\n");
            printf("   key       = %s\n", key);
            printf("   old value = %s\n", etcd_value);
            printf("   new value = %s\n", value);
            result = -1;
        }
        if (etcd_value) {
            free(etcd_value);
        }
    }
    if(always_write || !result) {
        result = etcd_set(key, value, ttl, false);
    }
    return result;
}


/**
 * etcd_watch
 */
int etcd_watch(const char* key, long long index, char** action, char** prevValue, char** value, char** rkey, long long* modifiedIndex) {
    json_error_t error;
    json_t* js_root = NULL;
    json_t* js_node = NULL;
    json_t* js_prevNode = NULL;
    json_t* js_action = NULL;
    json_t* js_value = NULL;
    json_t* js_rkey = NULL;
    json_t* js_prevValue = NULL;
    json_t* js_modIndex = NULL;
    int retVal = -1;
    char *url = NULL;
    int res;
    struct MemoryStruct reply;

    reply.memory = malloc(1); /* will be grown as needed by the realloc above */
    reply.size = 0; /* no data at this point */

    if (index != 0)
        asprintf(&url, "http://%s:%d/v2/keys/%s?wait=true&recursive=true&waitIndex=%lld", etcd_server, etcd_port, key, index);
    else
        asprintf(&url, "http://%s:%d/v2/keys/%s?wait=true&recursive=true", etcd_server, etcd_port, key);
    res = performRequest(url, GET, WriteMemoryCallback, NULL, (void*) &reply);
    if(url)
        free(url);
    if (res == CURLE_OK) {
        js_root = json_loads(reply.memory, 0, &error);

        if (js_root != NULL) {
            js_action = json_object_get(js_root, ETCD_JSON_ACTION);
            js_node = json_object_get(js_root, ETCD_JSON_NODE);
            js_prevNode = json_object_get(js_root, ETCD_JSON_PREVNODE);
            retVal = 0;
        }
        if (js_prevNode != NULL) {
            js_prevValue = json_object_get(js_prevNode, ETCD_JSON_VALUE);
        }
        if (js_node != NULL) {
            js_rkey = json_object_get(js_node, ETCD_JSON_KEY);
            js_value = json_object_get(js_node, ETCD_JSON_VALUE);
            js_modIndex = json_object_get(js_node, ETCD_JSON_MODIFIEDINDEX);
        }
        if (js_prevNode != NULL) {
            js_prevValue = json_object_get(js_prevNode, ETCD_JSON_VALUE);
        }
        if ((prevValue != NULL) && (js_prevValue != NULL) && (json_is_string(js_prevValue))) {

            *prevValue = strdup(json_string_value(js_prevValue));
        }
        if(modifiedIndex != NULL) {
            if ((js_modIndex != NULL) && (json_is_integer(js_modIndex))) {
                *modifiedIndex = json_integer_value(js_modIndex);
            } else {
                *modifiedIndex = index;
            }
        }
        if ((rkey != NULL) && (js_rkey != NULL) && (json_is_string(js_rkey))) {
            *rkey = strdup(json_string_value(js_rkey));

        }
        if ((action != NULL)  && (js_action != NULL)  && (json_is_string(js_action))) {
            *action = strdup(json_string_value(js_action));
        }
        if ((value != NULL) && (js_value != NULL) && (json_is_string(js_value))) {
            *value = strdup(json_string_value(js_value));
        }
        if (js_root != NULL) {
            json_decref(js_root);
        }

    }

    if (reply.memory) {
        free(reply.memory);
    }

    return retVal;
}

/**
 * etcd_del
 */
int etcd_del(const char* key) {
    json_error_t error;
    json_t* js_root = NULL;
    json_t* js_node = NULL;
    int retVal = -1;
    char *url;
    int res;
    struct MemoryStruct reply;

    reply.memory = malloc(1); /* will be grown as needed by the realloc above */
    reply.size = 0; /* no data at this point */

    asprintf(&url, "http://%s:%d/v2/keys/%s?recursive=true", etcd_server, etcd_port, key);
    res = performRequest(url, DELETE, WriteMemoryCallback, NULL, (void*) &reply);
    free(url);

    if (res == CURLE_OK) {
        js_root = json_loads(reply.memory, 0, &error);
        if (js_root != NULL) {
            js_node = json_object_get(js_root, ETCD_JSON_NODE);
        }

        if (js_node != NULL) {
            retVal = 0;
        }

        if (js_root != NULL) {
            json_decref(js_root);
        }
    }

    if (reply.memory) {
        free(reply.memory);
    }

    return retVal;
}


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static int performRequest(char* url, request_t request, void* callback, void* reqData, void* repData) {
    CURL *curl = NULL;
    CURLcode res = 0;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, DEFAULT_CURL_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, DEFAULT_CURL_CONECTTIMEOUT);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, repData);

    if (request == PUT) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqData);
    } else if (request == DELETE) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (request == GET) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    }

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}
