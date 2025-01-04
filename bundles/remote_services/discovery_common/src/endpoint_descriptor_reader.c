/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * endpoint_descriptor_reader.c
 *
 *  \date       24 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlreader.h>

#include "celix_log_helper.h"
#include "remote_constants.h"

#include "endpoint_description.h"
#include "endpoint_descriptor_common.h"
#include "endpoint_descriptor_reader.h"

struct endpoint_descriptor_reader {
    xmlTextReaderPtr reader;
    celix_log_helper_t **loghelper;
};

static valueType valueTypeFromString(char *name);

celix_status_t endpointDescriptorReader_create(endpoint_discovery_poller_t *poller, endpoint_descriptor_reader_t **reader) {
    celix_status_t status = CELIX_SUCCESS;

    *reader = malloc(sizeof(**reader));
    if (!*reader) {
        status = CELIX_ENOMEM;
    } else {
        (*reader)->reader = NULL;
        (*reader)->loghelper = poller->loghelper;
    }

    return status;
}

celix_status_t endpointDescriptorReader_destroy(endpoint_descriptor_reader_t *reader) {
    celix_status_t status = CELIX_SUCCESS;

    reader->loghelper = NULL;

    free(reader);

    return status;
}

void endpointDescriptorReader_addSingleValuedProperty(celix_properties_t *properties, const xmlChar* name, const xmlChar* value) {
    celix_properties_set(properties, (char *) name, (char*) value);
}

void endpointDescriptorReader_addMultiValuedProperty(celix_properties_t *properties, const xmlChar* name, celix_array_list_t* values) {
    char *value = calloc(256, sizeof(*value));
    if (value) {
        unsigned int size = celix_arrayList_size(values);
        unsigned int i;
        for (i = 0; i < size; i++) {
            char* item = (char*) celix_arrayList_get(values, i);
            if (i > 0) {
                value = strcat(value, ",");
            }
            value = strcat(value, item);
        }

        celix_properties_set(properties, (char *) name, value);

        free(value);
    }
}

celix_status_t endpointDescriptorReader_parseDocument(endpoint_descriptor_reader_t *reader, char *document, celix_array_list_t** endpoints) {
    celix_status_t status = CELIX_SUCCESS;

    reader->reader = xmlReaderForMemory(document, (int) strlen(document), NULL, "UTF-8", 0);
    if (reader->reader == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        bool inProperty = false;
        bool inXml = false;
        bool inArray = false;
        bool inList = false;
        bool inSet = false;
        bool inValue = false;

        const xmlChar *propertyName = NULL;
        const xmlChar *propertyValue = NULL;
        valueType propertyType = VALUE_TYPE_STRING;
        xmlChar *valueBuffer = xmlMalloc(256);
        valueBuffer[0] = '\0';

        celix_array_list_t* propertyValues = celix_arrayList_createPointerArray();

        celix_array_list_t* endpointDescriptions = NULL;
        if (*endpoints) {
            // use the given arraylist...
            endpointDescriptions = *endpoints;
        } else {
            endpointDescriptions = celix_arrayList_createPointerArray();
            // return the read endpoints...
            *endpoints = endpointDescriptions;
        }

        celix_properties_t *endpointProperties = NULL;

        int read = xmlTextReaderRead(reader->reader);
        while (read == XML_TEXTREADER_MODE_INTERACTIVE) {
            int type = xmlTextReaderNodeType(reader->reader);

            if (type == XML_READER_TYPE_ELEMENT) {
                const xmlChar *localname = xmlTextReaderConstLocalName(reader->reader);

                if (inXml) {
                    valueBuffer = xmlStrcat(valueBuffer, BAD_CAST "<");
                    valueBuffer = xmlStrcat(valueBuffer, localname);

                    int i = xmlTextReaderMoveToFirstAttribute(reader->reader);
                    while (i == 1) {
                        const xmlChar *name = xmlTextReaderConstName(reader->reader);
                        const xmlChar *value = xmlTextReaderConstValue(reader->reader);

                        valueBuffer = xmlStrcat(valueBuffer, BAD_CAST " ");
                        valueBuffer = xmlStrcat(valueBuffer, name);
                        valueBuffer = xmlStrcat(valueBuffer, BAD_CAST "=\"");
                        valueBuffer = xmlStrcat(valueBuffer, BAD_CAST value);
                        valueBuffer = xmlStrcat(valueBuffer, BAD_CAST "\"");

                        i = xmlTextReaderMoveToNextAttribute(reader->reader);
                    }

                    valueBuffer = xmlStrcat(valueBuffer, BAD_CAST ">");
                } else if (xmlStrcmp(localname, ENDPOINT_DESCRIPTION) == 0) {

                    if (endpointProperties != NULL)
                        celix_properties_destroy(endpointProperties);

                    endpointProperties = celix_properties_create();
                } else if (xmlStrcmp(localname, PROPERTY) == 0) {
                    inProperty = true;

                    propertyName = xmlTextReaderGetAttribute(reader->reader, NAME);
                    propertyValue = xmlTextReaderGetAttribute(reader->reader, VALUE);
                    xmlChar *vtype = xmlTextReaderGetAttribute(reader->reader, VALUE_TYPE);
                    propertyType = valueTypeFromString((char*) vtype);
                    celix_arrayList_clear(propertyValues);

                    if (xmlTextReaderIsEmptyElement(reader->reader)) {
                        inProperty = false;

                        if (propertyValue != NULL) {
                            endpointDescriptorReader_addSingleValuedProperty(endpointProperties, propertyName, propertyValue);
                        }

                        xmlFree((void *) propertyName);
                        xmlFree((void *) propertyValue);
                        xmlFree((void *) vtype);
                    }
                } else {
                    valueBuffer[0] = 0;
                    inArray |= inProperty && xmlStrcmp(localname, ARRAY) == 0;
                    inList |= inProperty && xmlStrcmp(localname, LIST) == 0;
                    inSet |= inProperty && xmlStrcmp(localname, SET) == 0;
                    inXml |= inProperty && xmlStrcmp(localname, XML) == 0;
                    inValue |= inProperty && xmlStrcmp(localname, VALUE) == 0;
                }
            } else if (type == XML_READER_TYPE_END_ELEMENT) {
                const xmlChar *localname = xmlTextReaderConstLocalName(reader->reader);

                if (inXml) {
                    if (xmlStrcmp(localname, XML) != 0)  {
                        valueBuffer = xmlStrcat(valueBuffer, BAD_CAST "</");
                        valueBuffer = xmlStrcat(valueBuffer, localname);
                        valueBuffer = xmlStrcat(valueBuffer, BAD_CAST ">");
                    }
                    else {
                        inXml = false;
                    }
                } else if (xmlStrcmp(localname, ENDPOINT_DESCRIPTION) == 0) {
                    endpoint_description_t *endpointDescription = NULL;
                    // Completely parsed endpoint description, add it to our list of results...
                    if(endpointDescription_create(endpointProperties, &endpointDescription) == CELIX_SUCCESS){
                        celix_arrayList_add(endpointDescriptions, endpointDescription);
                    }

                    endpointProperties = celix_properties_create();
                } else if (xmlStrcmp(localname, PROPERTY) == 0) {
                    inProperty = false;

                    if (inArray || inList || inSet) {
                        endpointDescriptorReader_addMultiValuedProperty(endpointProperties, propertyName, propertyValues);
                    }
                    else if (propertyValue != NULL) {
                        if (propertyType != VALUE_TYPE_STRING) {
                            celix_logHelper_warning(*reader->loghelper, "ENDPOINT_DESCRIPTOR_READER: Only string support for %s\n", propertyName);
                        }
                        endpointDescriptorReader_addSingleValuedProperty(endpointProperties, propertyName, propertyValue);

                        xmlFree((void *) propertyValue);
                    }
                    else {
                        endpointDescriptorReader_addSingleValuedProperty(endpointProperties, propertyName, valueBuffer);
                    }

                    xmlFree((void *) propertyName);
                    unsigned int k=0;
                    for(;k<celix_arrayList_size(propertyValues);k++){
                        free(celix_arrayList_get(propertyValues,k));
                    }
                    celix_arrayList_clear(propertyValues);

                    propertyType = VALUE_TYPE_STRING;
                    inArray = false;
                    inList = false;
                    inSet = false;
                    inXml = false;
                } else if (xmlStrcmp(localname, VALUE) == 0) {
                    celix_arrayList_add(propertyValues, strdup((char*) valueBuffer));
                    valueBuffer[0] = 0;
                    inValue = false;
                }
            } else if (type == XML_READER_TYPE_TEXT) {
                if (inValue || inXml) {
                    const xmlChar *value = xmlTextReaderValue(reader->reader);
                    valueBuffer = xmlStrcat(valueBuffer, value);
                    xmlFree((void *)value);
                }
            }

            read = xmlTextReaderRead(reader->reader);
        }

        if(endpointProperties!=NULL){
            celix_properties_destroy(endpointProperties);
        }

        unsigned int k=0;
        for(;k<celix_arrayList_size(propertyValues);k++){
            free(celix_arrayList_get(propertyValues,k));
        }
        celix_arrayList_destroy(propertyValues);
        xmlFree(valueBuffer);

        xmlFreeTextReader(reader->reader);
    }

    return status;
}

static valueType valueTypeFromString(char *name) {
    if (name == NULL || strcmp(name, "") == 0 || strcmp(name, "String") == 0) {
        return VALUE_TYPE_STRING;
    } else if (strcmp(name, "long") == 0 || strcmp(name, "Long") == 0) {
        return VALUE_TYPE_LONG;
    } else if (strcmp(name, "double") == 0 || strcmp(name, "Double") == 0) {
        return VALUE_TYPE_DOUBLE;
    } else if (strcmp(name, "float") == 0 || strcmp(name, "Float") == 0) {
        return VALUE_TYPE_FLOAT;
    } else if (strcmp(name, "int") == 0 || strcmp(name, "integer") == 0 || strcmp(name, "Integer") == 0) {
        return VALUE_TYPE_INTEGER;
    } else if (strcmp(name, "short") == 0 || strcmp(name, "Short") == 0) {
        return VALUE_TYPE_SHORT;
    } else if (strcmp(name, "byte") == 0 || strcmp(name, "Byte") == 0) {
        return VALUE_TYPE_BYTE;
    } else if (strcmp(name, "char") == 0 || strcmp(name, "Character") == 0) {
        return VALUE_TYPE_CHAR;
    } else if (strcmp(name, "boolean") == 0 || strcmp(name, "Boolean") == 0) {
        return VALUE_TYPE_BOOLEAN;
    } else {
        return VALUE_TYPE_STRING;
    }
}

