#ifndef MATH_COMPONENT_H_
#define MATH_COMPONENT_H_

#include "celix_errno.h"

typedef struct math_component *math_component_pt;

celix_status_t mathComponent_create(math_component_pt *math);
celix_status_t mathComponent_destroy(math_component_pt math);

int mathComponent_calc(math_component_pt math, int arg1, int arg2);


#endif /* MATH_COMPONENT_H_ */
