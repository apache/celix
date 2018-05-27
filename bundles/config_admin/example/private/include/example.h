#ifndef __EXAMPLE_H_
#define __EXAMPLE_H_



#include <celix_errno.h>
#include <array_list.h>
#include <properties.h>

typedef struct example *example_pt;

celix_status_t example_create(example_pt *component);
celix_status_t example_start(example_pt component);
celix_status_t example_stop(example_pt component);
celix_status_t example_destroy(example_pt component);
celix_status_t example_updated(example_pt component, properties_pt updatedProperties);

#endif //__EXAMPLE_H_
