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

static int dynInterface_parseSection(dyn_interface_type *intf, FILE *stream);
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

        char peek = fgetc(descriptor);
        while (peek == ':') {
            ungetc(peek, descriptor);
            status = dynInterface_parseSection(intf, descriptor);
            if (status == OK) {
                peek = fgetc(descriptor);
            } else {
                break;
            }
        }

        if (status == OK) {
            status = dynCommon_eatChar(descriptor, EOF);
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

static int dynInterface_parseSection(dyn_interface_type *intf, FILE *stream) {
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
        if (strcmp("annotations", sectionName) ==0) {
                status = dynInterface_parseAnnotations(intf, stream);
        } else if (strcmp("types", sectionName) == 0) {
                status =dynInterface_parseTypes(intf, stream);
        } else if (strcmp("methods", sectionName) == 0) {
                status =dynInterface_parseMethods(intf, stream);
        } else {
            status = ERROR;
            LOG_ERROR("unsupported section '%s'", sectionName);
        }
    }

    return status;
}

static int dynInterface_parseAnnotations(dyn_interface_type *intf, FILE *stream) {
    int status = OK;

    int peek = fgetc(stream);
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        char *name;
        char *value;
        status = dynCommon_parseNameValue(stream, &name, &value);

        if (status == OK) {
            status = dynCommon_eatChar(stream, '\n');
        }

        struct namval_entry *entry = NULL;
        if (status == OK) {
            entry = calloc(1, sizeof(*entry));
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
            if (entry != NULL) {
                free(entry);
            }
            break;
        }
        peek = fgetc(stream);
    }
    ungetc(peek, stream);
    
    return status;
}

static int dynInterface_parseTypes(dyn_interface_type *intf, FILE *stream) {
    int status = OK;

    //expected input (Name)=<Type>\n
    int peek = fgetc(stream);
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        char *name;
        status = dynCommon_parseName(stream, &name);

        if (status == OK) {
            status = dynCommon_eatChar(stream, '=');
        }

        dyn_type *type = NULL;
        if (status == OK) {
            dynType_parse(stream, name, &intf->types, &type);
        }
        if (name != NULL) {
            free(name);
        }

        if (status == OK) {
            status = dynCommon_eatChar(stream, '\n');
        }

        struct type_entry *entry = NULL;
        if (status == OK) {
            entry = calloc(1, sizeof(*entry));
            if (entry != NULL) {
                entry->type = type;
                TAILQ_INSERT_TAIL(&intf->types, entry, entries);
            } else {
                status = ERROR;
                LOG_ERROR("Error allocating memory for type entry");
            }
        }

        if (status != OK) {
            if (type != NULL) {
                dynType_destroy(type);
            }
            if (entry != NULL) {
                free(entry);
            }
            break;
        }
        peek = fgetc(stream);
    }
    ungetc(peek, stream);

    return status;
}

static int dynInterface_parseMethods(dyn_interface_type *intf, FILE *stream) {
    int status = OK;

    //expected input (Name)=<Method>\n
    int peek = fgetc(stream);
    int index = 0;
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        char *id;
        status = dynCommon_parseNameAlsoAccept(stream, "();[{}", &id);

        if (status == OK) {
            status = dynCommon_eatChar(stream, '=');
        }


        dyn_function_type *func = NULL;
        if (status == OK) {
            status = dynFunction_parse(stream, &intf->types, &func);
        }

        if (status == OK) {
            status = dynCommon_eatChar(stream, '\n');
        }

        struct method_entry *entry = NULL;
        if (status == OK) {
            entry = calloc(1, sizeof(*entry));
            if (entry != NULL) {
                entry->index = index++;
                entry->id = id;
                entry->dynFunc = func;
                TAILQ_INSERT_TAIL(&intf->methods, entry, entries);
            } else {
                status = ERROR;
                LOG_ERROR("Error allocating memory for method entry");
            }
        }

        if (status != OK) {
            if (id != NULL) {
                free(id);
            }
            if (func != NULL) {
                dynFunction_destroy(func);
                //TODO free strIdentier, name
            }
            if (entry != NULL) {
                free(entry);
            }
            break;
        }
        peek = fgetc(stream);
    }
    ungetc(peek, stream);

    return status;
}

void dynInterface_destroy(dyn_interface_type *intf) {
    if (intf != NULL) {
        struct namval_entry *nTmp = NULL;
        struct namval_entry *nEntry = TAILQ_FIRST(&intf->annotations);
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

        struct type_entry *tmp = NULL;
        struct type_entry *tInfo = TAILQ_FIRST(&intf->types);
        while (tInfo != NULL) {
            tmp = tInfo;
            tInfo = TAILQ_NEXT(tInfo, entries);
            dynType_destroy(tmp->type);
            free(tmp);
        }

        struct method_entry *mTmp = NULL;
        struct method_entry *mInfo = TAILQ_FIRST(&intf->methods);
        while (mInfo != NULL) {
            mTmp = mInfo;
            mInfo = TAILQ_NEXT(mInfo, entries);
            
            if (mTmp->id != NULL) {
                free(mTmp->id);
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
    struct namval_entry *entry = NULL;
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
