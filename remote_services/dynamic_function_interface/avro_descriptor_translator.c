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

static const int OK = 0;
static const int MEM_ERROR = 1;
static const int PARSE_ERROR = 2;
static const int INV_ARG_ERROR = 2;
                   
static int descriptorTranslator_createMethodDescriptor(interface_descriptor_type *desc, json_t *schema, const char *name, int index, json_t *message); 
static int descriptorTranslator_parseMessage(json_t *schema, const char *name, json_t *message, bool asJavaSignature, char **descriptor); 
static int descriptorTranslator_parseArgument(FILE *stream, json_t *schema, json_t *type);
static int descriptorTranslator_parseType(FILE *stream, json_t *schema, const char *typeName); 

int descriptorTranslator_create(const char *schemaStr, interface_descriptor_type **out) {
    int status = OK;

    interface_descriptor_type *desc = calloc(1, sizeof(*desc));
    if (desc != NULL) {
        TAILQ_INIT(&desc->methodDescriptors);
        json_error_t error;
        json_t *schema = json_loads(schemaStr, JSON_DECODE_ANY, &error);

        if (schema != NULL) {
            //TODO
            json_t *messages = json_object_get(schema, "messages");
            if (messages != NULL) {
                const char *name;
                json_t *message;
                int rc = 0;
                int index = 0;
                json_object_foreach(messages, name, message) {
                   rc = descriptorTranslator_createMethodDescriptor(desc, schema, name, index++, message); 
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


    if (status == 0) { 
        *out = desc;
    } else if (status == MEM_ERROR) {
        printf("AVRO_DESCRIPTOR_TRANSLATOR: error cannot allocate memory\n");
        descriptorTranslator_destroy(desc);
    } else  {
        descriptorTranslator_destroy(desc);
    }
    return status;
}

int descriptorTranslator_destroy(interface_descriptor_type *desc) {
    int status = OK;
    if (desc != NULL) {
        //TODO free existing members
        free(desc);
    } else {
        status = INV_ARG_ERROR;
    }
    return status;
}

static int descriptorTranslator_createMethodDescriptor(interface_descriptor_type *desc, json_t *schema, const char *name, int index, json_t *message) {
    int status = OK;

    method_descriptor_type *mDesc = calloc(1, sizeof(*mDesc));
    if (mDesc != NULL) {
        mDesc->identifier = index;
        status = descriptorTranslator_parseMessage(schema, name, message, false, &mDesc->descriptor);
        if (status == OK) {
            mDesc->name = strdup(name);
            if (mDesc->name == NULL) {
                status = MEM_ERROR;
            } else {
                status = descriptorTranslator_parseMessage(schema, name, message, true, &mDesc->strIdentifier);
            }
        }
    } else {
        status = MEM_ERROR;
    }

    if (status == OK) {
        TAILQ_INSERT_TAIL(&desc->methodDescriptors, mDesc, entries);
    } else {
        if (mDesc != NULL) {
            if (mDesc->name != NULL) {
                free(mDesc->name);
            }
            if (mDesc->descriptor != NULL) {
                free(mDesc->descriptor);
            }
            if (mDesc->strIdentifier != NULL) {
                free(mDesc->strIdentifier);
            }
            free(mDesc);
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
                    status = descriptorTranslator_parseArgument(memStream, schema, type);
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
            status = descriptorTranslator_parseArgument(memStream, schema, response);
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

static int descriptorTranslator_parseArgument(FILE *stream, json_t *schema, json_t *type) {
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
            status = descriptorTranslator_parseType(stream, schema, typeStr);
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
            descriptorTranslator_parseArgument(stream, schema, items);
        } else {
            printf("sub type %s not supported\n", json_string_value(subType));
            status = PARSE_ERROR;
        }
    }
    return status;
}
            

static int descriptorTranslator_parseType(FILE *stream, json_t *schema, const char *typeName) {
    int status = OK;

    json_t *types = json_object_get(schema, "types");
    json_t *type = NULL;
    if (json_is_array(types)) {
        json_t *el;
        int index;
        json_array_foreach(types, index, el) {
            const char *name = json_string_value(json_object_get(el, "name"));
            if (strcmp(typeName, name) == 0) {
                type = el;
                break;
            }
        }

        if (el != NULL) {
            fputc('{', stream);
            json_t *fields = json_object_get(type, "fields");
            if (json_is_array(fields)) {
                json_t *field;
                json_array_foreach(fields, index, field) {
                    json_t *type = json_object_get(field, "type");
                    status = descriptorTranslator_parseArgument(stream, schema, type);
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
        } else {
            status = PARSE_ERROR;
            printf("cannot find type with name %s\n", typeName);
        }
    } else {
        status = PARSE_ERROR;
        printf("Expected array type\n");
    }

    return status;
}
