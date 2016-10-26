#include <stdlib.h>

#include <pthread.h>

#include "hash_map.h"

#include "example.h"

struct example {
};

//Create function
celix_status_t example_create(example_pt *result) {
	celix_status_t status = CELIX_SUCCESS;
	
    example_pt component = calloc(1, sizeof(*component));
	if (component != NULL) {
		(*result) = component;
	} else {
		status = CELIX_ENOMEM;
	}	
	return status;
}

celix_status_t example_destroy(example_pt component) {
	celix_status_t status = CELIX_SUCCESS;
	if (component != NULL) {
		free(component);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	return status;
}

celix_status_t example_start(example_pt component) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t example_stop(example_pt component) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t example_updated(example_pt component, properties_pt updatedProperties) {
    printf("updated called\n");
    hash_map_pt map = (hash_map_pt)updatedProperties;
    if (map != NULL) {
        hash_map_iterator_pt iter = hashMapIterator_create(map);
        while(hashMapIterator_hasNext(iter)) {
            char *key = hashMapIterator_nextKey(iter);
            const char *value = properties_get(updatedProperties, key);
            printf("got property %s:%s\n", key, value);
        }
    } else {
        printf("updated with NULL properties\n");
    }
    return CELIX_SUCCESS;
}
