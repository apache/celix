#ifndef ETCD_H_
#define ETCD_H_

#define MAX_NODES			256

#define MAX_KEY_LENGTH		256
#define MAX_VALUE_LENGTH	256
#define MAX_ACTION_LENGTH	64

#define MAX_URL_LENGTH		256
#define MAX_CONTENT_LENGTH	1024

#define ETCD_JSON_NODE			"node"
#define ETCD_JSON_PREVNODE		"prevNode"
#define ETCD_JSON_NODES			"nodes"
#define ETCD_JSON_ACTION		"action"
#define ETCD_JSON_KEY			"key"
#define ETCD_JSON_VALUE			"value"
#define ETCD_JSON_MODIFIEDINDEX	"modifiedIndex"

bool etcd_init(char* server, int port);
bool etcd_get(char* key, char* value, char*action, int* modifiedIndex);
bool etcd_getNodes(char* directory, char** nodeNames, int* size);
bool etcd_set(char* key, char* value, int ttl);
bool etcd_del(char* key);
bool etcd_watch(char* key, int index, char* action, char* prevValue, char* value);

#endif /* ETCD_H_ */
