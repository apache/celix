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

#include "array_list.h"

//celix_status_t edr_create() {
//
//}

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

int main() {
    xmlTextReaderPtr reader = xmlReaderForFile("desc.xml", NULL, 0);
    if (reader != NULL) {
        bool inProperty = false;
        bool inXml = false;
        bool inArray = false;
        bool inList = false;
        bool inSet = false;
        bool inValue = false;

        const char *propertyName = NULL;
        valueType propertyType;
        const char *propertyValue = NULL;
        const char *value = NULL;
        char *valueBuffer = malloc(256);
        valueBuffer[0] = '\0';
        unsigned int currentSize = 255;

        array_list_pt propertyValues = NULL;
        arrayList_create(&propertyValues);

        int read = xmlTextReaderRead(reader);
        while (read == 1) {
            int type = xmlTextReaderNodeType(reader);

            if (type == 1) {
                const char *localname = (const char*) xmlTextReaderConstLocalName(reader);

                if (inXml) {
                    if (strlen(valueBuffer) + strlen(localname) + 1 > currentSize) {
                        valueBuffer = realloc(*valueBuffer, currentSize * 2);
                    }
                    strcat(valueBuffer, "<");
                    strcat(valueBuffer, localname);
    //                m_valueBuffer.append("<" + qName);
    //                for (int i = 0; i < attributes.getLength(); i++) {
    //                    m_valueBuffer.append(" ").append(attributes.getQName(i)).append("=\"")
    //                        .append(attributes.getValue(i)).append("\"");
    //                }
    //                m_valueBuffer.append(">");
                    strcat(valueBuffer, ">");
                    read = xmlTextReaderRead(reader);
                    continue;
                }

                if (strcmp(localname, "property") == 0) {
                    inProperty = true;
                    propertyName = (const char *) xmlTextReaderGetAttribute(reader, "name");
                    propertyType = getValueType((const char *) xmlTextReaderGetAttribute(reader, "value-type"));
                    propertyValue = (const char *) xmlTextReaderGetAttribute(reader, "value");
                    arrayList_clear(propertyValues);

                    read = xmlTextReaderRead(reader);
                    continue;
                }

                valueBuffer[0] = '\0';
                value = NULL;
                inArray |= inProperty && strcmp(localname, "array") == 0;
                inList |= inProperty && strcmp(localname, "list") == 0;
                inSet |= inProperty && strcmp(localname, "set") == 0;
                inXml |= inProperty && strcmp(localname, "xml") == 0;
                inValue |= inProperty && strcmp(localname, "value") == 0;
            }

            if (type == 15) {
                const char *localname = (const char*) xmlTextReaderConstLocalName(reader);

                if (inXml) {
                    if (strcmp(localname, "xml") != 0)  {
                        strcat(valueBuffer, "</");
                        strcat(valueBuffer, localname);
                        strcat(valueBuffer, ">");
                    }
                    else {
                        inXml = false;
                    }
                    read = xmlTextReaderRead(reader);
                    continue;
                }

                if (strcmp(localname, "endpoint-description") == 0) {
//                    m_endpointDesciptions.add(new EndpointDescription(m_endpointProperties));
                    printf("New description\n");
                    read = xmlTextReaderRead(reader);
                    continue;
                }

                if (strcmp(localname, "property") == 0) {
                    inProperty = false;

                    printf("Property: %s, %d, %s\n", propertyName, propertyType, propertyValue);

                    if (inArray) {
//                        m_endpointProperties.put(m_propertyName, getPropertyValuesArray());
                    }
                    else if (inList) {
//                        m_endpointProperties.put(m_propertyName, getPropertyValuesList());
                    }
                    else if (inSet) {
//                        m_endpointProperties.put(m_propertyName, getPropertyValuesSet());
                    }
                    else if (propertyValue != NULL) {
//                        m_endpointProperties.put(m_propertyName, m_propertyType.parse(m_propertyValue));
                    }
                    else {
                        printf("Buffer: %s\n", valueBuffer);
//                        m_endpointProperties.put(m_propertyName, m_valueBuffer.toString());
                    }
                    inArray = false;
                    inList = false;
                    inSet = false;
                    inXml = false;
                    read = xmlTextReaderRead(reader);
                    continue;
                }

                if (strcmp(localname, "value") == 0) {
//                    m_propertyValues.add(m_propertyType.parse(m_valueBuffer.toString()));
                    inValue = false;
                    read = xmlTextReaderRead(reader);
                    continue;
                }
            }

            if (type == 3) {
                if (inValue || inXml) {
                    const char *value = (const char*) xmlTextReaderValue(reader);
                    printf("Value: %s\n", value);
                    strcat(valueBuffer, value);
//                    m_valueBuffer.append(ch, start, length);
                }
            }

            read = xmlTextReaderRead(reader);
        }
    }

    return 0;
}

static valueType getValueType(char *name) {
    if (name == NULL || strcmp(name, "") == 0) {
        return VALUE_TYPE_STRING;
    }
    if (strcmp(name, "String") == 0) {
        return VALUE_TYPE_STRING;
    } else if (strcmp(name, "long") == 0 || strcmp(name, "Long") == 0) {
        return VALUE_TYPE_LONG;
    } else if (strcmp(name, "double") == 0 || strcmp(name, "Double") == 0) {
        return VALUE_TYPE_DOUBLE;
    } else if (strcmp(name, "float") == 0 || strcmp(name, "Float") == 0) {
        return VALUE_TYPE_FLOAT;
    } else if (strcmp(name, "integer") == 0 || strcmp(name, "Integer") == 0) {
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
