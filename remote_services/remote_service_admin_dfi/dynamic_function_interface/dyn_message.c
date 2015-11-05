/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "dyn_message.h"

#include <stdlib.h>
#include <string.h>

#include "dyn_common.h"
#include "dyn_type.h"

DFI_SETUP_LOG(dynMessage);

struct _dyn_message_type {
    struct namvals_head header;
    struct namvals_head annotations;
    struct types_head types;
    dyn_type *msgType;
};

static const int OK = 0;
static const int ERROR = 1;

static int dynMessage_parseSection(dyn_message_type *msg, FILE *stream);
static int dynMessage_parseAnnotations(dyn_message_type *msg, FILE *stream);
static int dynMessage_parseTypes(dyn_message_type *msg, FILE *stream);
static int dynMessage_parseMessage(dyn_message_type *msg, FILE *stream);
static int dynMessage_parseHeader(dyn_message_type *msg, FILE *stream);
static int dynMessage_parseNameValueSection(dyn_message_type *msg, FILE *stream, struct namvals_head *head);
static int dynMessage_checkMessage(dyn_message_type *msg);
static int dynMessage_getEntryForHead(struct namvals_head *head, const char *name, char **value);

int dynMessage_parse(FILE *descriptor, dyn_message_type **out) {
    int status = OK;

    dyn_message_type *msg = calloc(1, sizeof(*msg));
    if (msg != NULL) {
        TAILQ_INIT(&msg->header);
        TAILQ_INIT(&msg->annotations);
        TAILQ_INIT(&msg->types);

        char peek = fgetc(descriptor);
        while (peek == ':') {
            ungetc(peek, descriptor);
            status = dynMessage_parseSection(msg, descriptor);
            if (status == OK) {
                peek = fgetc(descriptor);
            } else {
                break;
            }
        }

        if (status == OK) {
            status = dynCommon_eatChar(descriptor, EOF);
        }

        if (status == OK) {
            status = dynMessage_checkMessage(msg);
        }
    } else {
        status = ERROR;
        LOG_ERROR("Error allocating memory for dynamic message\n");
    }

    if (status == OK) {
        *out = msg;
    } else if (msg != NULL) {
        dynMessage_destroy(msg);
    }
    return status;
}

static int dynMessage_checkMessage(dyn_message_type *msg) {
    int status = OK;

    //check header section
    if (status == OK) {
        bool foundType = false;
        bool foundVersion = false;
        bool foundName = false;
        struct namval_entry *entry = NULL;
        TAILQ_FOREACH(entry, &msg->header, entries) {
            if (strcmp(entry->name, "type") == 0) {
                foundType = true;
            } else if (strcmp(entry->name, "version") == 0) {
                foundVersion = true;
            } else if (strcmp(entry->name, "name") == 0) {
                foundName = true;
            }
        }

        if (!foundType || !foundVersion || !foundName) {
            status = ERROR;
            LOG_ERROR("Parse Error. There must be a header section with a type, version and name entry");
        }
    }

    return status;
}

static int dynMessage_parseSection(dyn_message_type *msg, FILE *stream) {
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
        if (strcmp("header", sectionName) == 0) {
            status = dynMessage_parseHeader(msg, stream);
        } else if (strcmp("annotations", sectionName) == 0) {
            status = dynMessage_parseAnnotations(msg, stream);
        } else if (strcmp("types", sectionName) == 0) {
            status = dynMessage_parseTypes(msg, stream);
        } else if (strcmp("message", sectionName) == 0) {
            status = dynMessage_parseMessage(msg, stream);
        } else {
            status = ERROR;
            LOG_ERROR("unsupported section '%s'", sectionName);
        }
    }

    if (sectionName != NULL) {
        free(sectionName);
    }

    return status;
}

static int dynMessage_parseHeader(dyn_message_type *msg, FILE *stream) {
    return dynMessage_parseNameValueSection(msg, stream, &msg->header);
}

static int dynMessage_parseAnnotations(dyn_message_type *msg, FILE *stream) {
    return dynMessage_parseNameValueSection(msg, stream, &msg->annotations);
}

static int dynMessage_parseNameValueSection(dyn_message_type *msg, FILE *stream, struct namvals_head *head) {
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
                TAILQ_INSERT_TAIL(head, entry, entries);
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

static int dynMessage_parseTypes(dyn_message_type *msg, FILE *stream) {
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
            dynType_parse(stream, name, &msg->types, &type);
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
                TAILQ_INSERT_TAIL(&msg->types, entry, entries);
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

static int dynMessage_parseMessage(dyn_message_type *msg, FILE *stream) {
    int status = OK;

    //expected input <dynType>\n
    char *name = NULL;
    status = dynMessage_getName(msg, &name);

    if (status == OK) {
    	status = dynType_parse(stream, name, &(msg->types), &(msg->msgType));
    }

    return status;
}

void dynMessage_destroy(dyn_message_type *msg) {
    if (msg != NULL) {
        dynCommon_clearNamValHead(&msg->header);
        dynCommon_clearNamValHead(&msg->annotations);

        struct type_entry *tmp = NULL;
        struct type_entry *tInfo = TAILQ_FIRST(&msg->types);
        while (tInfo != NULL) {
            tmp = tInfo;
            tInfo = TAILQ_NEXT(tInfo, entries);
            dynType_destroy(tmp->type);
            free(tmp);
        }

        if (msg->msgType != NULL) {
        	dynType_destroy(msg->msgType);
        }

        free(msg);
    } 
}

int dynMessage_getName(dyn_message_type *msg, char **out) {
    return dynMessage_getEntryForHead(&msg->header, "name", out);
}

int dynMessage_getVersion(dyn_message_type *msg, char **version) {
    return dynMessage_getEntryForHead(&msg->header, "version", version);
}

int dynMessage_getHeaderEntry(dyn_message_type *msg, const char *name, char **value) {
    return dynMessage_getEntryForHead(&msg->header, name, value);
}

int dynMessage_getAnnotationEntry(dyn_message_type *msg, const char *name, char **value) {
    return dynMessage_getEntryForHead(&msg->annotations, name, value);
}

static int dynMessage_getEntryForHead(struct namvals_head *head, const char *name, char **out) {
    int status = OK;
    char *value = NULL;
    struct namval_entry *entry = NULL;
    TAILQ_FOREACH(entry, head, entries) {
        if (strcmp(name, entry->name) == 0) {
            value = entry->value;
            break;
        }
    }
    if (value != NULL) {
        *out = value;
    } else {
        status = ERROR;
        LOG_WARNING("Cannot find '%s' in list", name);
    }
    return status;
}

int dynMessage_getMessageType(dyn_message_type *msg, dyn_type **type) {
	int status = OK;
	*type = msg->msgType;
	return status;
}
