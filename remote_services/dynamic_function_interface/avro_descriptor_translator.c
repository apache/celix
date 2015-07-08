/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "descriptor_translator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <jansson.h>

#if defined(BSD) || defined(__APPLE__) 
#include "open_memstream.h"
#include "fmemopen.h"
#endif

DFI_SETUP_LOG(descriptorTranslator)

static const int OK = 0;
static const int MEM_ERROR = 1;
static const int PARSE_ERROR = 2;
static const int INV_ARG_ERROR = 2;
                   
static int descriptorTranslator_createMethodInfo(dyn_interface_type *intf, json_t *schema, const char *name, int index, json_t *message); 
static int descriptorTranslator_parseType(dyn_interface_type *intf, json_t *type);
static int descriptorTranslator_parseMessage(json_t *schema, const char *name, json_t *message, bool asJavaSignature, char **descriptor); 
static int descriptorTranslator_parseArgument(FILE *stream, json_t *type);

int descriptorTranslator_translate(const char *schemaStr, dyn_interface_type **out) {
    LOG_DEBUG("translating descriptor for schema '%s'\n", schemaStr);
    int status = OK;

    dyn_interface_type *intf = NULL;
    status = dynInterface_create("TODO", &intf);
    if (status == 0) {
        json_error_t error;
        json_t *schema = json_loads(schemaStr, JSON_DECODE_ANY, &error);

        if (schema != NULL) {
            json_t *types = json_object_get(schema, "types");
            if (types != NULL) {
                json_t *type = NULL;
                int index = 0;
                json_array_foreach(types, index, type) {
                    descriptorTranslator_parseType(intf, type);
                }

            }
            json_t *messages = json_object_get(schema, "messages");
            if (messages != NULL) {
                const char *name;
                json_t *message;
                int rc = 0;
                int index = 0;
                json_object_foreach(messages, name, message) {
                   rc = descriptorTranslator_createMethodInfo(intf, schema, name, index++, message); 
                   if (rc != OK) {
                       break;
                   }
                }
            }
            //json_decref(schema);
        } else {
            status = PARSE_ERROR;
            printf("AVRO_DESCRIPTOR_TRANSLATOR: error parsing json input '%s'. Error is %s\n", schemaStr, error.text);
        }
    } else { 
        status = MEM_ERROR;
    }


    if (status == OK) { 
        *out = intf;
    } else {
        dynInterface_destroy(intf);
    }
    return status;
}

static int descriptorTranslator_createMethodInfo(dyn_interface_type *intf, json_t *schema, const char *name, int index, json_t *message) {
    int status = OK;

    method_info_type *mInfo = calloc(1, sizeof(*mInfo));
    if (mInfo != NULL) {
        mInfo->identifier = index;
        status = descriptorTranslator_parseMessage(schema, name, message, false, &mInfo->descriptor);
        if (status == OK) {
            mInfo->name = strdup(name);
            if (mInfo->name == NULL) {
                status = MEM_ERROR;
            } else {
                status = descriptorTranslator_parseMessage(schema, name, message, true, &mInfo->strIdentifier);
            }
        }
    } else {
        status = MEM_ERROR;
    }

    if (status == OK) {
        TAILQ_INSERT_TAIL(&intf->methodInfos, mInfo, entries);
    } else {
        if (mInfo != NULL) {
            if (mInfo->name != NULL) {
                free(mInfo->name);
            }
            if (mInfo->descriptor != NULL) {
                free(mInfo->descriptor);
            }
            if (mInfo->strIdentifier != NULL) {
                free(mInfo->strIdentifier);
            }
            free(mInfo);
        }
    }

    return status;
} 

static int descriptorTranslator_parseMessage(json_t *schema, const char *name, json_t *message, bool asJavaSignature, char **descriptor) {
    int status = OK;
    //message -> { "request" : [ {"name":"<name>", "type":"<type>"} * ], "response":"<type>" }
    //array -> "type":"array", "items:"<type>"

    char *ptr = NULL;
    size_t ptrSize;
    FILE *memStream = open_memstream(&ptr, &ptrSize);
    
    if (memStream != NULL) { 
        json_t *request = json_object_get(message, "request");
        fwrite(name, 1, strlen(name), memStream);
        fputc('(', memStream);
        if (!asJavaSignature) {
            fputc('P', memStream); //handle
        }
    
        if (request != NULL) {
            size_t index;
            json_t *arg;
            json_array_foreach(request, index, arg) {
                //json_t *name = json_object_get(arg, "name");
                json_t *type = json_object_get(arg, "type");
                if (type != NULL) {
                    status = descriptorTranslator_parseArgument(memStream, type);
                } else {
                    printf("expected type for request argument %zu for message %s\n", index, name);
                    status = PARSE_ERROR;
                }
                if (status != OK) { 
                    break;
                }
            }
        } else {
            status = PARSE_ERROR;
            printf("Expected request for message %s\n", name);    
        }

        json_t *response = json_object_get(message, "response");
        if (status == OK && response != NULL) {
            if (asJavaSignature) {
                fputc(')', memStream);
            } else {
                fputc('*', memStream); //output parameter
            }
            status = descriptorTranslator_parseArgument(memStream, response);
        } 

        if (!asJavaSignature) {
            fputc(')', memStream);
            fputc('N', memStream); //error / exceptions
        }
   } else {
       status = MEM_ERROR;
   }

    if (memStream != NULL) {
        fclose(memStream);
        if (status == OK) {
            *descriptor = ptr;
        } else {
            free(ptr);
        }
    } 

    return status;
}

static const char * const PRIMITIVE_INT = "int";
static const char * const PRIMITIVE_LONG = "long";
static const char * const PRIMITIVE_STRING = "string";
static const char * const PRIMITIVE_BOOL = "boolean";
static const char * const PRIMITIVE_FLOAT = "float";
static const char * const PRIMITIVE_DOUBLE = "double";
static const char * const PRIMITIVE_NULL = "null";
static const char * const PRIMITIVE_BYTES = "bytes";

static int descriptorTranslator_parseArgument(FILE *stream, json_t *type) {
    int status = OK;
    if (json_is_string(type)) {
        const char *typeStr = json_string_value(type);
        char t = '\0';
        if (strcmp(typeStr, PRIMITIVE_INT) == 0) {
            t = 'I';
        } else if (strcmp(typeStr, PRIMITIVE_LONG) == 0) {
            t = 'J';
        } else if (strcmp(typeStr, PRIMITIVE_STRING) == 0) {
            t = 'T';
        } else if (strcmp(typeStr, PRIMITIVE_BOOL) == 0) {
            t = 'Z';
        } else if (strcmp(typeStr, PRIMITIVE_FLOAT) == 0) {
            t = 'F';
        } else if (strcmp(typeStr, PRIMITIVE_DOUBLE) == 0) {
            t = 'D';
        } else if (strcmp(typeStr, PRIMITIVE_NULL) == 0) {
            t = 'V';
        } else if (strcmp(typeStr, PRIMITIVE_BYTES) == 0) {
            t = 'B';
        } else {
            fprintf(stream, "L%s;", typeStr);
        }
        if (t != '\0') {
            fputc(t, stream);
        }
    } else {
        json_t *subType = json_object_get(type, "type");
        json_t *items = json_object_get(type, "items");
        if (strcmp("array", json_string_value(subType)) == 0) {
            //array
            fputc('[', stream);
            descriptorTranslator_parseArgument(stream, items);
        } else {
            printf("sub type %s not supported\n", json_string_value(subType));
            status = PARSE_ERROR;
        }
    }
    return status;
}
            
static int descriptorTranslator_parseType(dyn_interface_type *intf, json_t *type) {
    int status = OK;
    const char *name = json_string_value(json_object_get(type, "name"));
    type_info_type *tInfo = NULL;

    char *ptr = NULL;
    size_t ptrSize;
    FILE *stream = open_memstream(&ptr, &ptrSize);

    if (stream != NULL) {
        fputc('{', stream);
        json_t *fields = json_object_get(type, "fields");
        if (json_is_array(fields)) {
            int index = 0;
            json_t *field = NULL;
            json_array_foreach(fields, index, field) {
                json_t *type = json_object_get(field, "type");
                status = descriptorTranslator_parseArgument(stream, type);
                if (status != OK) { 
                    break;
                }
            }
            if (status == OK) {
                json_array_foreach(fields, index, field) {
                    const char *fieldName = json_string_value(json_object_get(field, "name"));
                    if (fieldName != NULL) { 
                        fputc(' ', stream);
                        fwrite(fieldName, 1, strlen(fieldName), stream);
                    } else {
                        status = PARSE_ERROR;
                        printf("Expected name for field\n");
                        break;
                    }
                }
            }
        } else {
            status = PARSE_ERROR;
            printf("Expected array type");
        }
        fputc('}', stream);
        fclose(stream);
    } else {
        status = MEM_ERROR;
        LOG_ERROR("Error creating memory stream");
    }

    if (status == OK) {
        tInfo = calloc(1, sizeof(*tInfo));
        if (tInfo != NULL) {
           tInfo->name = strdup(name);
           if (tInfo->name != NULL) {
               tInfo->descriptor = ptr;
           } else {
               status = MEM_ERROR;
               LOG_ERROR("Error allocating memory for type info name");
           }
        } else {
            status = MEM_ERROR;
            LOG_ERROR("Error allocating memory for type_info");
        }
    }

    if (status != 0 ) {
        if (tInfo != NULL) {
            if (tInfo->name != NULL) {
                free(tInfo->name);
            }
            free(tInfo);
        }
        if (ptr != NULL) {
            free(ptr);
        }
    }

    if (status == OK) {
        TAILQ_INSERT_TAIL(&intf->typeInfos, tInfo, entries);
    }

    return status;
}
