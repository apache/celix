#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>
#include <jansson.h>

#include "etcd.h"

#define DEFAULT_CURL_TIMEOUT          10
#define DEFAULT_CURL_CONECTTIMEOUT    10

typedef enum {
	GET, PUT, DELETE
} request_t;

static char* etcd_server = NULL;
static int etcd_port = 0;

struct MemoryStruct {
	char *memory;
	size_t size;
};

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
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, DEFAULT_CURL_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, DEFAULT_CURL_CONECTTIMEOUT);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, repData);

	if (request == PUT) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
//		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, "Content-type: application/json");
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

// open
bool etcd_init(char* server, int port) {
	etcd_server = server;
	etcd_port = port;

	return curl_global_init(CURL_GLOBAL_ALL) == 0;
}

// get
bool etcd_get(char* key, char* value, char* action, int* modifiedIndex) {
	json_t* js_root = NULL;
	json_t* js_node = NULL;
	json_t* js_value = NULL;
	json_t* js_modifiedIndex = NULL;
	json_error_t error;
	int res;
	struct MemoryStruct reply;

	reply.memory = malloc(1); /* will be grown as needed by the realloc above */
	reply.size = 0; /* no data at this point */

	bool retVal = false;
	char url[MAX_URL_LENGTH];
	snprintf(url, MAX_URL_LENGTH, "http://%s:%d/v2/keys/%s", etcd_server, etcd_port, key);

	res = performRequest(url, GET, WriteMemoryCallback, NULL, (void*) &reply);

	if (res == CURLE_OK) {
		js_root = json_loads(reply.memory, 0, &error);

		if (js_root != NULL) {
			js_node = json_object_get(js_root, ETCD_JSON_NODE);
		}
		if (js_node != NULL) {
			js_value = json_object_get(js_node, ETCD_JSON_VALUE);
			js_modifiedIndex = json_object_get(js_node, ETCD_JSON_MODIFIEDINDEX);

			*modifiedIndex = json_integer_value(js_modifiedIndex);
			strncpy(value, json_string_value(js_value), MAX_VALUE_LENGTH);
			retVal = true;
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

// getNodes
bool etcd_getNodes(char* directory, char** nodeNames, int* size) {
	json_t* js_root = NULL;
	json_t* js_node = NULL;
	json_t* js_nodes = NULL;
	json_error_t error;
	int res;
	struct MemoryStruct reply;

	reply.memory = malloc(1); /* will be grown as needed by the realloc above */
	reply.size = 0; /* no data at this point */

	bool retVal = false;
	char url[MAX_URL_LENGTH];
	snprintf(url, MAX_URL_LENGTH, "http://%s:%d/v2/keys/%s", etcd_server, etcd_port, directory);

	res = performRequest(url, GET, WriteMemoryCallback, NULL, (void*) &reply);

	if (res == CURLE_OK) {
		js_root = json_loads(reply.memory, 0, &error);

		if (js_root != NULL) {
			js_node = json_object_get(js_root, ETCD_JSON_NODE);
		}
		if (js_root != NULL) {
			js_nodes = json_object_get(js_node, ETCD_JSON_NODES);
		}


		if (js_nodes != NULL && json_is_array(js_nodes)) {
			int i = 0;
			retVal = true;

			for (i = 0; i < json_array_size(js_nodes) && i < MAX_NODES; i++) {
				json_t* js_node = json_array_get(js_nodes, i);

				if (!json_is_object(js_node)) {
					retVal = false;
				} else {
					json_t* js_key = json_object_get(js_node, ETCD_JSON_KEY);
					strncpy(nodeNames[i], json_string_value(js_key), MAX_KEY_LENGTH);
				}
			}
			*size = i;
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



bool etcd_set(char* key, char* value, int ttl, bool prevExist) {
	json_error_t error;
	json_t* js_root = NULL;
	json_t* js_node = NULL;
	json_t* js_value = NULL;
	bool retVal = false;
	char url[MAX_URL_LENGTH];
	char request[MAX_CONTENT_LENGTH];
	char* cur = request;
	int res;
	struct MemoryStruct reply;

	reply.memory = malloc(1); /* will be grown as needed by the realloc above */
	reply.size = 0; /* no data at this point */

	snprintf(url, MAX_URL_LENGTH, "http://%s:%d/v2/keys/%s", etcd_server, etcd_port, key);
	cur += snprintf(cur, MAX_CONTENT_LENGTH, "value=%s", value);

	if (ttl > 0)
	    cur += snprintf(cur, MAX_CONTENT_LENGTH, ";ttl=%d", ttl);

	if (prevExist)
	    cur += snprintf(cur, MAX_CONTENT_LENGTH, ";prevExist=true");

	res = performRequest(url, PUT, WriteMemoryCallback, request, (void*) &reply);

	if (res == CURLE_OK) {
		js_root = json_loads(reply.memory, 0, &error);

		if (js_root != NULL) {
			js_node = json_object_get(js_root, ETCD_JSON_NODE);
		}
		if (js_node != NULL) {
			js_value = json_object_get(js_node, ETCD_JSON_VALUE);
		}
		if (js_value != NULL && json_is_string(js_value)) {
			retVal = (strcmp(json_string_value(js_value), value) == 0);
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



//delete
bool etcd_del(char* key) {
	json_error_t error;
	json_t* js_root = NULL;
	json_t* js_node = NULL;
	bool retVal = false;
	char url[MAX_URL_LENGTH];
	char request[MAX_CONTENT_LENGTH];
	int res;
	struct MemoryStruct reply;

	reply.memory = malloc(1); /* will be grown as needed by the realloc above */
	reply.size = 0; /* no data at this point */

	snprintf(url, MAX_URL_LENGTH, "http://%s:%d/v2/keys/%s", etcd_server, etcd_port, key);
	res = performRequest(url, DELETE, WriteMemoryCallback, request, (void*) &reply);

	if (res == CURLE_OK) {
		js_root = json_loads(reply.memory, 0, &error);

		if (js_root != NULL) {
			js_node = json_object_get(js_root, ETCD_JSON_NODE);
		}

		retVal = (js_node != NULL);

		if (js_root != NULL) {
			json_decref(js_root);
		}
	}

	if (reply.memory) {
		free(reply.memory);
	}


	return retVal;
}

///watch
bool etcd_watch(char* key, int index, char* action, char* prevValue, char* value) {
	json_error_t error;
	json_t* js_root = NULL;
	json_t* js_node = NULL;
	json_t* js_prevNode = NULL;
	json_t* js_action = NULL;
	json_t* js_value = NULL;
	json_t* js_prevValue = NULL;
	bool retVal = false;
	char url[MAX_URL_LENGTH];
	int res;
	struct MemoryStruct reply;

	reply.memory = malloc(1); /* will be grown as needed by the realloc above */
	reply.size = 0; /* no data at this point */

	if (index != 0)
		snprintf(url, MAX_URL_LENGTH, "http://%s:%d/v2/keys/%s?wait=true&recursive=true&waitIndex=%d", etcd_server, etcd_port, key,
				index);
	else
		snprintf(url, MAX_URL_LENGTH, "http://%s:%d/v2/keys/%s?wait=true&recursive=true", etcd_server, etcd_port, key);

	res = performRequest(url, GET, WriteMemoryCallback, NULL, (void*) &reply);

	if (res == CURLE_OK) {
		js_root = json_loads(reply.memory, 0, &error);

		if (js_root != NULL) {
			js_action = json_object_get(js_root, ETCD_JSON_ACTION);
			js_node = json_object_get(js_root, ETCD_JSON_NODE);
			js_prevNode = json_object_get(js_root, ETCD_JSON_PREVNODE);
		}
		if (js_prevNode != NULL) {
			js_prevValue = json_object_get(js_prevNode, ETCD_JSON_VALUE);
		}
		if (js_node != NULL) {
			js_value = json_object_get(js_node, ETCD_JSON_VALUE);
		}
		if (js_prevNode != NULL) {
			js_prevValue = json_object_get(js_prevNode, ETCD_JSON_VALUE);
		}
		if ((js_prevValue != NULL) && (json_is_string(js_prevValue))) {
			strncpy(prevValue, json_string_value(js_prevValue), MAX_VALUE_LENGTH);
		}
		if ((js_value != NULL) && (json_is_string(js_value))) {
			strncpy(value, json_string_value(js_value), MAX_VALUE_LENGTH);
		}
		if ((js_action != NULL) && (json_is_string(js_action))) {
			strncpy(action, json_string_value(js_action), MAX_ACTION_LENGTH);

			retVal = true;
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
