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
#include <string.h>
#include <libxml/xmlreader.h>

#include "constants.h"
#include "remote_constants.h"

#include "endpoint_description.h"
#include "endpoint_descriptor_common.h"
#include "endpoint_descriptor_reader.h"
#include "properties.h"
#include "utils.h"

struct endpoint_descriptor_reader {
    xmlTextReaderPtr reader;
};

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

void endpointDescriptorReader_addSingleValuedProperty(properties_pt properties, const xmlChar* name, const xmlChar* value) {
	properties_set(properties, strdup((char *) name), strdup((char *) value));
}

void endpointDescriptorReader_addMultiValuedProperty(properties_pt properties, const xmlChar* name, array_list_pt values) {
	char value[256];
	value[0] = '\0';
    int i, size = arrayList_size(values);
    for (i = 0; i < size; i++) {
        char* item = (char*) arrayList_get(values, i);
        if (i > 0) {
            strcat(value, ",");
        }
        strcat(value, item);
    }

    properties_set(properties, strdup((char *) name), strdup(value));
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
        const xmlChar *propertyValue = NULL;
        valueType propertyType = VALUE_TYPE_STRING;
        xmlChar *valueBuffer = xmlMalloc(256);
        valueBuffer[0] = '\0';
        unsigned int currentSize = 255;

        array_list_pt propertyValues = NULL;
        arrayList_create(&propertyValues);

        array_list_pt endpointDescriptions = NULL;
        if (*endpoints) {
        	// use the given arraylist...
        	endpointDescriptions = *endpoints;
        } else {
			arrayList_create(&endpointDescriptions);
			// return the read endpoints...
			*endpoints = endpointDescriptions;
        }

        properties_pt endpointProperties = NULL;

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
                    endpointProperties = properties_create();
                } else if (xmlStrcmp(localname, PROPERTY) == 0) {
                    inProperty = true;

                    propertyName = xmlTextReaderGetAttribute(reader->reader, NAME);
                    propertyValue = xmlTextReaderGetAttribute(reader->reader, VALUE);
                    xmlChar* type = xmlTextReaderGetAttribute(reader->reader, VALUE_TYPE);
                    propertyType = valueTypeFromString((char*) type);
                    arrayList_clear(propertyValues);

                    if (xmlTextReaderIsEmptyElement(reader->reader)) {
                        inProperty = false;

                        if (propertyValue != NULL) {
                        	if (propertyType != VALUE_TYPE_STRING && strcmp(OSGI_RSA_ENDPOINT_SERVICE_ID, (char*) propertyName)) {
                        		printf("ENDPOINT_DESCRIPTOR_READER: Only single-valued string supported for %s\n", propertyName);
                        	}
                        	endpointDescriptorReader_addSingleValuedProperty(endpointProperties, propertyName, propertyValue);
                        }

                        xmlFree((void *) propertyName);
                        xmlFree((void *) propertyValue);
                        xmlFree((void *) type);
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
                    endpoint_description_pt endpointDescription = NULL;
                    // Completely parsed endpoint description, add it to our list of results...
                    endpointDescription_create(endpointProperties, &endpointDescription);
                    arrayList_add(endpointDescriptions, endpointDescription);

                    endpointProperties = properties_create();
                } else if (xmlStrcmp(localname, PROPERTY) == 0) {
                    inProperty = false;

                    if (inArray || inList || inSet) {
						endpointDescriptorReader_addMultiValuedProperty(endpointProperties, propertyName, propertyValues);
                    }
                    else if (propertyValue != NULL) {
                    	if (propertyType != VALUE_TYPE_STRING) {
                    		printf("ENDPOINT_DESCRIPTOR_READER: Only string support for %s\n", propertyName);
                    	}
                    	endpointDescriptorReader_addSingleValuedProperty(endpointProperties, propertyName, propertyValue);

                        xmlFree((void *) propertyValue);
                    }
                    else {
                    	endpointDescriptorReader_addSingleValuedProperty(endpointProperties, propertyName, valueBuffer);
                    }

                    xmlFree((void *) propertyName);
                    arrayList_clear(propertyValues);

                    propertyType = VALUE_TYPE_STRING;
                    inArray = false;
                    inList = false;
                    inSet = false;
                    inXml = false;
                } else if (xmlStrcmp(localname, VALUE) == 0) {
                    arrayList_add(propertyValues, strdup((char*) valueBuffer));
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

        arrayList_destroy(propertyValues);
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

#ifdef RSA_ENDPOINT_TEST_READER
int main() {
    array_list_pt list = NULL;
    endpoint_descriptor_reader_pt reader = NULL;

    char *doc = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<endpoint-descriptions xmlns=\"http://www.osgi.org/xmlns/rsa/v1.0.0\">"
    "<endpoint-description>"
    	"<property name=\"endpoint.service.id\" value-type=\"long\" value=\"6\"/>"
		"<property name=\"endpoint.framework.uuid\" value=\"2983D849-93B1-4C2C-AC6D-5BCDA93ACB96\"/>"
        "<property name=\"service.intents\">"
            "<list>"
                "<value>SOAP</value>"
                "<value>HTTP</value>"
            "</list>"
        "</property>"
        "<property name=\"endpoint.id\" value=\"11111111-1111-1111-1111-111111111111\" />"
        "<property name=\"objectClass\"><array><value>com.acme.Foo</value></array></property>"
        "<property name=\"endpoint.package.version.com.acme\" value=\"4.2\" />"
        "<property name=\"service.imported.configs\" value=\"com.acme\" />"
    	"<property name=\"service.imported\" value=\"true\"/>"
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
        	"<property name=\"endpoint.service.id\" value-type=\"long\" value=\"5\"/>"
    		"<property name=\"endpoint.framework.uuid\" value=\"2983D849-93B1-4C2C-AC6D-5BCDA93ACB96\"/>"
			"<property name=\"service.intents\">"
				"<list>"
					"<value>SOAP</value>"
					"<value>HTTP</value>"
				"</list>"
			"</property>"
			"<property name=\"endpoint.id\" value=\"22222222-2222-2222-2222-222222222222\" />"
            "<property name=\"objectClass\"><array><value>com.acme.Bar</value></array></property>"
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

	endpointDescriptorReader_create(&reader);

	endpointDescriptorReader_parseDocument(reader, doc, &list);

	int i;
	for (i = 0; i < arrayList_size(list); i++) {
		printf("\nEndpoint description #%d:\n", (i+1));
		endpoint_description_pt edp = arrayList_get(list, i);
		printf("Id: %s\n", edp->id);
		printf("Service Id: %ld\n", edp->serviceId);
		printf("Framework UUID: %s\n", edp->frameworkUUID);
		printf("Service: %s\n", edp->service);

		properties_pt props = edp->properties;
		if (props) {
			printf("Service properties:\n");
			hash_map_iterator_pt iter = hashMapIterator_create(props);
			while (hashMapIterator_hasNext(iter)) {
				hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

				printf("- %s => '%s'\n", hashMapEntry_getKey(entry), hashMapEntry_getValue(entry));
			}
			hashMapIterator_destroy(iter);
		} else {
			printf("No service properties...\n");
		}


		endpointDescription_destroy(edp);
	}

	if (list != NULL) {
		arrayList_destroy(list);
	}

	endpointDescriptorReader_destroy(reader);

    return 0;
}
#endif
