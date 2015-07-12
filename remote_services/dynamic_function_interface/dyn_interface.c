/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "dyn_interface.h"

#include <stdlib.h>
#include <string.h>

#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_interface.h"

DFI_SETUP_LOG(dynInterface);

const int OK = 0;
const int ERROR = 1;

static int dynInterface_parseAnnotations(dyn_interface_type *intf, FILE *stream);
static int dynInterface_parseTypes(dyn_interface_type *intf, FILE *stream);
static int dynInterface_parseMethods(dyn_interface_type *intf, FILE *stream);

int dynInterface_parse(FILE *descriptor, dyn_interface_type **out) {
    int status = OK;

    dyn_interface_type *intf = calloc(1, sizeof(*intf));
    if (intf != NULL) {
        TAILQ_INIT(&intf->annotations);
        TAILQ_INIT(&intf->types);
        TAILQ_INIT(&intf->methods);

        status = dynInterface_parseAnnotations(intf, descriptor);
        if (status == OK) {
            status =dynInterface_parseTypes(intf, descriptor);
        }
        if (status == OK) {
            status = dynInterface_parseMethods(intf, descriptor);
        }
    } else {
        status = ERROR;
        LOG_ERROR("Error allocating memory for dynamic interface\n");
    }

    if (status == OK) {
        *out = intf;
    } else if (intf != NULL) {
        dynInterface_destroy(intf);
    }
    return status;
}

static int dynInterface_parseAnnotations(dyn_interface_type *intf, FILE *stream) {
    int status = OK;
    char *sectionName = NULL;

    status = dynCommon_eatChar(stream, ':');

    if (status == OK) {
        status = dynCommon_parseName(stream, &sectionName);
    }

    if (status == OK) {
        status = dynCommon_eatChar(stream, '\n');
    }

    if (status == OK) {
        if (strcmp("annotations", sectionName) == 0) {
            LOG_DEBUG("Parsed annotations section header");
        } else {
            status = ERROR;
            LOG_ERROR("Expected annotations section, got '%s'", sectionName);
        }
    }

    if (status == OK) {
        int peek = fgetc(stream);
        while (peek != ':' && peek != EOF) {
            ungetc(peek, stream);

            char *name;
            char *value;
            status = dynCommon_parseNameValue(stream, &name, &value);

            if (status == OK) {
                status = dynCommon_eatChar(stream, '\n');
            }

            if (status == OK) {
                interface_namval_type *entry = calloc(1, sizeof(*entry));
                if (entry != NULL) {
                    entry->name = name;
                    entry->value = value;
                    TAILQ_INSERT_TAIL(&intf->annotations, entry, entries);
                } else {
                    status = ERROR;
                    LOG_ERROR("Error allocating memory for namval entry");
                }
            }

            if (status != OK) {
                if (name != NULL) {
                    free(name);
                }
                if (value != NULL) {
                    free(value);
                }
                break;
            }
            peek = fgetc(stream);
        }
        ungetc(peek, stream);
    }
    
    return status;
}

static int dynInterface_parseTypes(dyn_interface_type *intf, FILE *stream) {
    int status = OK;
    //TODO implement -> extract section parse part  from parseAnnotations first?
    return status;
}

static int dynInterface_parseMethods(dyn_interface_type *intf, FILE *stream) {
    int status = OK;
    //TODO refactor
    return status;
}

void dynInterface_destroy(dyn_interface_type *intf) {
    if (intf != NULL) {
        interface_namval_type *nTmp = NULL;
        interface_namval_type *nEntry = TAILQ_FIRST(&intf->annotations);
        while (nEntry != NULL) {
            nTmp = nEntry;
            nEntry = TAILQ_NEXT(nEntry, entries);
            if (nTmp->name != NULL) {
                free(nTmp->name);
            }
            if (nTmp->value != NULL) {
                free(nTmp->value);
            }
            free(nTmp);
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

//TODO refactor using a dynInterface_findAnnotation method
int dynInterface_getName(dyn_interface_type *intf, char **out) {
    int status = OK;
    char *name = NULL;
    interface_namval_type *entry = NULL;
    TAILQ_FOREACH(entry, &intf->annotations, entries) {
        if (strcmp("name", entry->name) == 0) {
            name = entry->value;
            break;
        }
    }

    if (name != NULL) {
        *out = name;
    } else {
        status = ERROR;
        LOG_WARNING("Cannot find 'name' in dyn interface annotations");
    }
    return status;
}
