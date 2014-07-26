/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * endpoint_description_reader.c
 *
 *  \date       24 Jul 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <libxml/xmlreader.h>
#include <string.h>

#include "endpoint_description.h"
#include "endpoint_descriptor_reader.h"
#include "properties.h"
#include "utils.h"

struct endpoint_descriptor_reader {
    xmlTextReaderPtr reader;
};

typedef enum {
    VALUE_TYPE_STRING,
    VALUE_TYPE_LONG,
    VALUE_TYPE_DOUBLE,
    VALUE_TYPE_FLOAT,
    VALUE_TYPE_INTEGER,
    VALUE_TYPE_BYTE,
    VALUE_TYPE_CHAR,
    VALUE_TYPE_BOOLEAN,
    VALUE_TYPE_SHORT,
} valueType;


static valueType getValueType(char *name);

celix_status_t endpointDescriptorReader_create(endpoint_descriptor_reader_pt *reader) {
    celix_status_t status = CELIX_SUCCESS;

    *reader = malloc(sizeof(**reader));
    if (!reader) {
        status = CELIX_ENOMEM;
    } else {
        (*reader)->reader = NULL;
    }

    return status;
}

celix_status_t endpointDescriptorReader_destroy(endpoint_descriptor_reader_pt reader) {
    celix_status_t status = CELIX_SUCCESS;

    free(reader);

    return status;
}

celix_status_t endpointDescriptorReader_parseDocument(endpoint_descriptor_reader_pt reader, char *document, array_list_pt *endpoints) {
    celix_status_t status = CELIX_SUCCESS;

    reader->reader = xmlReaderForMemory(document, strlen(document), NULL, "UTF-8", 0);
    if (reader == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        bool inProperty = false;
        bool inXml = false;
        bool inArray = false;
        bool inList = false;
        bool inSet = false;
        bool inValue = false;

        const xmlChar *propertyName = NULL;
        const xmlChar *propertyType = NULL;
        const xmlChar *propertyValue = NULL;
        const xmlChar *value = NULL;
        xmlChar *valueBuffer = xmlMalloc(256);
        valueBuffer[0] = '\0';
        unsigned int currentSize = 255;

//        array_list_pt propertyValues = NULL;
//        arrayList_create(&propertyValues);

        array_list_pt endpointDescriptions = NULL;
        arrayList_create(&endpointDescriptions);

        properties_pt endpointProperties = NULL;

        int read = xmlTextReaderRead(reader->reader);
        while (read == 1) {
            int type = xmlTextReaderNodeType(reader->reader);

            if (type == 1) {
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
//                    read = xmlTextReaderRead(reader);
//                    continue;
                } else if (xmlStrcmp(localname, BAD_CAST "endpoint-description") == 0) {
                    endpointProperties = properties_create();
                } else if (xmlStrcmp(localname, BAD_CAST "property") == 0) {
                    inProperty = true;
                    propertyName = xmlTextReaderGetAttribute(reader->reader, BAD_CAST "name");
//                    propertyType = getValueType((const char *) xmlTextReaderGetAttribute(reader, "value-type"));
//                    propertyType = xmlTextReaderGetAttribute(reader->reader, BAD_CAST "value-type");
                    propertyValue = xmlTextReaderGetAttribute(reader->reader, BAD_CAST "value");
//                    arrayList_clear(propertyValues);

                    if (xmlTextReaderIsEmptyElement(reader->reader)) {
                        inProperty = false;

                        if (propertyValue != NULL) {
                            printf("Only string support\n");
    //                        m_endpointProperties.put(m_propertyName, m_propertyType.parse(m_propertyValue));
                            properties_set(endpointProperties, (char *) propertyName, (char *) propertyValue);
                        }

                        free((void *) propertyName);
                        free((void *) propertyValue);
                    }

                    //                    read = xmlTextReaderRead(reader);
//                    continue;
                } else {
                    valueBuffer[0] = '\0';
                    value = NULL;
                    inArray |= inProperty && xmlStrcmp(localname, BAD_CAST "array") == 0;
                    inList |= inProperty && xmlStrcmp(localname, BAD_CAST "list") == 0;
                    inSet |= inProperty && xmlStrcmp(localname, BAD_CAST "set") == 0;
                    inXml |= inProperty && xmlStrcmp(localname, BAD_CAST "xml") == 0;
                    inValue |= inProperty && xmlStrcmp(localname, BAD_CAST "value") == 0;
                }
            }

            if (type == 15) {
                const xmlChar *localname = xmlTextReaderConstLocalName(reader->reader);

                if (inXml) {
                    if (xmlStrcmp(localname, BAD_CAST "xml") != 0)  {
                        xmlStrcmp(valueBuffer, BAD_CAST "</");
                        xmlStrcmp(valueBuffer, localname);
                        xmlStrcmp(valueBuffer, BAD_CAST ">");
                    }
                    else {
                        inXml = false;
                    }
//                    read = xmlTextReaderRead(reader);
//                    continue;
                } else if (xmlStrcmp(localname, BAD_CAST "endpoint-description") == 0) {
                    endpoint_description_pt endpointDescription = NULL;
                    endpointDescription_create(endpointProperties, &endpointDescription);
                    arrayList_add(endpointDescriptions, endpointDescription);
//                    endpointProperties = properties_create();
//                    hashMap_clear(endpointProperties, false, false);
//                    read = xmlTextReaderRead(reader);
//                    continue;
                } else if (xmlStrcmp(localname, BAD_CAST "property") == 0) {
                    inProperty = false;

                    if (inArray) {
                        printf("No array support\n");
//                        m_endpointProperties.put(m_propertyName, getPropertyValuesArray());
                    }
                    else if (inList) {
                        printf("No list support\n");
//                        m_endpointProperties.put(m_propertyName, getPropertyValuesList());
                    }
                    else if (inSet) {
                        printf("No set support\n");
//                        m_endpointProperties.put(m_propertyName, getPropertyValuesSet());
                    }
                    else if (propertyValue != NULL) {
                        printf("Only string support\n");
//                        m_endpointProperties.put(m_propertyName, m_propertyType.parse(m_propertyValue));
                        properties_set(endpointProperties, (char *) propertyName, (char *) propertyValue);

                        free((void *) propertyValue);
                    }
                    else {
                        properties_set(endpointProperties, (char *) propertyName, (char *) valueBuffer);
                    }

                    free((void *) propertyName);

                    inArray = false;
                    inList = false;
                    inSet = false;
                    inXml = false;
//                    read = xmlTextReaderRead(reader);
//                    continue;
                } else if (xmlStrcmp(localname, BAD_CAST "value") == 0) {
//                    m_propertyValues.add(m_propertyType.parse(m_valueBuffer.toString()));
//                    arrayList_add(propertyValues, valueBuffer);
                    inValue = false;
//                    read = xmlTextReaderRead(reader);
//                    continue;
                }
            }

            if (type == 3) {
                if (inValue || inXml) {
                    const xmlChar *value = xmlTextReaderValue(reader->reader);
                    printf("Value: %s\n", value);
                    valueBuffer = xmlStrcat(valueBuffer, value);
                    xmlFree((void *)value);
                }
            }

            read = xmlTextReaderRead(reader->reader);
        }
        *endpoints = endpointDescriptions;

//        arrayList_destroy(propertyValues);
        free(valueBuffer);
        xmlFreeTextReader(reader->reader);
    }

    return status;
}

//static valueType getValueType(char *name) {
//    if (name == NULL || strcmp(name, "") == 0) {
//        return VALUE_TYPE_STRING;
//    }
//    if (strcmp(name, "String") == 0) {
//        return VALUE_TYPE_STRING;
//    } else if (strcmp(name, "long") == 0 || strcmp(name, "Long") == 0) {
//        return VALUE_TYPE_LONG;
//    } else if (strcmp(name, "double") == 0 || strcmp(name, "Double") == 0) {
//        return VALUE_TYPE_DOUBLE;
//    } else if (strcmp(name, "float") == 0 || strcmp(name, "Float") == 0) {
//        return VALUE_TYPE_FLOAT;
//    } else if (strcmp(name, "integer") == 0 || strcmp(name, "Integer") == 0) {
//        return VALUE_TYPE_INTEGER;
//    } else if (strcmp(name, "short") == 0 || strcmp(name, "Short") == 0) {
//        return VALUE_TYPE_SHORT;
//    } else if (strcmp(name, "byte") == 0 || strcmp(name, "Byte") == 0) {
//        return VALUE_TYPE_BYTE;
//    } else if (strcmp(name, "char") == 0 || strcmp(name, "Character") == 0) {
//        return VALUE_TYPE_CHAR;
//    } else if (strcmp(name, "boolean") == 0 || strcmp(name, "Boolean") == 0) {
//        return VALUE_TYPE_BOOLEAN;
//    } else {
//        return VALUE_TYPE_STRING;
//    }
//}

int main() {
    array_list_pt list = NULL;
    endpoint_descriptor_reader_pt reader = NULL;
    endpointDescriptorReader_create(&reader);

    char *doc = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<endpoint-descriptions xmlns=\"http://www.osgi.org/xmlns/rsa/v1.0.0\">"
    "<endpoint-description>"
        "<property name=\"service.intents\">"
            "<list>"
                "<value>SOAP</value>"
                "<value>HTTP</value>"
            "</list>"
        "</property>"
        "<property name=\"endpoint.id\" value=\"http://ws.acme.com:9000/hello\" />"
        "<property name=\"objectClass\" value=\"com.acme.Foo\" />"
        "<property name=\"endpoint.package.version.com.acme\" value=\"4.2\" />"
        "<property name=\"service.imported.configs\" value=\"com.acme\" />"
        "<property name=\"com.acme.ws.xml\">"
            "<xml>"
                "<config xmlns=\"http://acme.com/defs\">"
                    "<port>1029</port>"
                    "<host>www.acme.com</host>"
                "</config>"
            "</xml>"
        "</property>"
    "</endpoint-description>"
            "<endpoint-description>"
                    "<property name=\"service.intents\">"
                        "<list>"
                            "<value>SOAP</value>"
                            "<value>HTTP</value>"
                        "</list>"
                    "</property>"
                    "<property name=\"endpoint.id\" value=\"http://ws.acme.com:9000/goodbye\" />"
                    "<property name=\"objectClass\" value=\"com.acme.Doo\" />"
                    "<property name=\"endpoint.package.version.com.acme\" value=\"4.2\" />"
                    "<property name=\"service.imported.configs\" value=\"com.acme\" />"
                    "<property name=\"com.acme.ws.xml\">"
                        "<xml>"
                            "<config xmlns=\"http://acme.com/defs\">"
                                "<port>1029</port>"
                                "<host>www.acme.com</host>"
                            "</config>"
                        "</xml>"
                    "</property>"
                "</endpoint-description>"
"</endpoint-descriptions>";

    endpointDescriptorReader_parseDocument(reader, doc, &list);

    int i;
    for (i = 0; i < arrayList_size(list); i++) {
        endpoint_description_pt edp = arrayList_get(list, i);
        printf("Service: %s\n", edp->service);
        printf("Id: %s\n", edp->id);

        endpointDescription_destroy(edp);
    }

    if (list != NULL) {
        arrayList_destroy(list);
    }

    endpointDescriptorReader_destroy(reader);

    return 0;
}
