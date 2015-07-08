/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "dyn_interface.h"

#include <stdlib.h>
#include <string.h>

DFI_SETUP_LOG(dynInterface)

int dynInterface_create(const char *name, dyn_interface_type **out) {
    int status = 0;
    dyn_interface_type *inft = calloc(1, sizeof(*inft));
    if (inft != NULL) {
        inft->name = strdup(name);
    }
    if (inft == NULL || inft->name == NULL) {
        status = 1;
        LOG_ERROR("Error allocating memory for dynamic interface\n");
    }

    if (status == 0) {
        TAILQ_INIT(&inft->typeInfos);
        TAILQ_INIT(&inft->methodInfos);
    }
    *out = inft;
    return status;
}

void dynInterface_destroy(dyn_interface_type *intf) {
    if (intf != NULL) {
        if (intf->name != NULL) {
            free(intf->name);
        }
        type_info_type *tInfo = NULL;
        TAILQ_FOREACH(tInfo, &intf->typeInfos, entries) {
            LOG_WARNING("TODO");
            //TODO add destroy func for type_info
        }
        method_info_type *mInfo = NULL;
        TAILQ_FOREACH(mInfo, &intf->typeInfos, entries) {
            LOG_WARNING("TODO");
            //TODO add destroy func for method
        }
    } 
}

