#include <stdlib.h>

#include <pthread.h>

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
    return CELIX_SUCCESS;
}
