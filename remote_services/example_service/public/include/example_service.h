/*
 * example_service.h
 *
 *  Created on: Oct 5, 2011
 *      Author: alexander
 */

#ifndef EXAMPLE_SERVICE_H_
#define EXAMPLE_SERVICE_H_

#define EXAMPLE_SERVICE "example"

typedef struct example *example_t;

typedef struct example_service *example_service_t;

struct example_service {
	example_t example;
	celix_status_t (*add)(example_t example, double a, double b, double *result);
	celix_status_t (*sub)(example_t example, double a, double b, double *result);
	celix_status_t (*sqrt)(example_t example, double a, double *result);
};

#endif /* EXAMPLE_SERVICE_H_ */
