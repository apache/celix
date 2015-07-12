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
        TAILQ_INIT(&inft->types);
        TAILQ_INIT(&inft->methods);
    }
    *out = inft;
    return status;
}

void dynInterface_destroy(dyn_interface_type *intf) {
    if (intf != NULL) {
        if (intf->name != NULL) {
            free(intf->name);
        }

        interface_type_type *tmp = NULL;
        interface_type_type *tInfo = TAILQ_FIRST(&intf->types);
        while (tInfo != NULL) {
            tmp = tInfo;
            tInfo = TAILQ_NEXT(tInfo, entries);

            if (tmp->name != NULL) {
                free(tmp->name);
            }

            free(tmp);
        }

        interface_method_type *mTmp = NULL;
        interface_method_type *mInfo = TAILQ_FIRST(&intf->methods);
        while (mInfo != NULL) {
            mTmp = mInfo;
            mInfo = TAILQ_NEXT(mInfo, entries);
            
            if (mTmp->strIdentifier != NULL) {
                free(mTmp->strIdentifier);
            }
            if (mTmp->name != NULL) {
                free(mTmp->name);
            }
            if (mTmp->dynFunc != NULL) {
                dynFunction_destroy(mTmp->dynFunc);
            }
            free(mTmp);
        }

        free(intf);
    } 
}

