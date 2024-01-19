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
 * endpoint_descriptor_writer.c
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <libxml/xmlwriter.h>

#include "celix_constants.h"
#include "remote_constants.h"

#include "endpoint_description.h"
#include "endpoint_descriptor_common.h"
#include "endpoint_descriptor_writer.h"

struct endpoint_descriptor_writer {
    xmlBufferPtr buffer;
    xmlTextWriterPtr writer;
};

static celix_status_t endpointDescriptorWriter_writeEndpoint(endpoint_descriptor_writer_t *writer, endpoint_description_t *endpoint);

static char* valueTypeToString(valueType type);

celix_status_t endpointDescriptorWriter_create(endpoint_descriptor_writer_t **writer) {
    celix_status_t status = CELIX_SUCCESS;

    *writer = malloc(sizeof(**writer));
    if (!*writer) {
        status = CELIX_ENOMEM;
    } else {
        (*writer)->buffer = xmlBufferCreate();
        if ((*writer)->buffer == NULL) {
            status = CELIX_BUNDLE_EXCEPTION;
        } else {
            (*writer)->writer = xmlNewTextWriterMemory((*writer)->buffer, 0);
            if ((*writer)->writer == NULL) {
                status = CELIX_BUNDLE_EXCEPTION;
            }
        }
    }

    return status;
}

celix_status_t endpointDescriptorWriter_destroy(endpoint_descriptor_writer_t *writer) {
    xmlFreeTextWriter(writer->writer);
    xmlBufferFree(writer->buffer);
    free(writer);
    return CELIX_SUCCESS;
}

celix_status_t endpointDescriptorWriter_writeDocument(endpoint_descriptor_writer_t *writer, array_list_pt endpoints, char **document) {
    celix_status_t status = CELIX_SUCCESS;
    int rc;

    rc = xmlTextWriterStartDocument(writer->writer, NULL, "UTF-8", NULL);
    if (rc < 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        rc = xmlTextWriterStartElementNS(writer->writer, NULL, ENDPOINT_DESCRIPTIONS, XMLNS);
        if (rc < 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        } else {
            unsigned int i;
            for (i = 0; i < arrayList_size(endpoints); i++) {
                endpoint_description_t *endpoint = arrayList_get(endpoints, i);
                status = endpointDescriptorWriter_writeEndpoint(writer, endpoint);
            }
            if (status == CELIX_SUCCESS) {
                rc = xmlTextWriterEndElement(writer->writer);
                if (rc < 0) {
                    status = CELIX_BUNDLE_EXCEPTION;
                } else {
                    rc = xmlTextWriterEndDocument(writer->writer);
                    if (rc < 0) {
                        status = CELIX_BUNDLE_EXCEPTION;
                    } else {
                        *document = (char *) writer->buffer->content;
                    }
                }
            }
        }
    }

    return status;
}

static celix_status_t endpointDescriptorWriter_writeArrayValue(xmlTextWriterPtr writer, const xmlChar* value) {
    xmlTextWriterStartElement(writer, ARRAY);
    xmlTextWriterStartElement(writer, VALUE);
    xmlTextWriterWriteString(writer, value);
    xmlTextWriterEndElement(writer); // value
    xmlTextWriterEndElement(writer); // array

	return CELIX_SUCCESS;
}

static celix_status_t endpointDescriptorWriter_writeTypedValue(xmlTextWriterPtr writer, valueType type, const xmlChar* value) {
	xmlTextWriterWriteAttribute(writer, VALUE_TYPE, (const xmlChar*) valueTypeToString(type));
	xmlTextWriterWriteAttribute(writer, VALUE, value);

	return CELIX_SUCCESS;
}

static celix_status_t endpointDescriptorWriter_writeUntypedValue(xmlTextWriterPtr writer, const xmlChar* value) {
	xmlTextWriterWriteAttribute(writer, VALUE, value);

	return CELIX_SUCCESS;
}

static celix_status_t endpointDescriptorWriter_writeEndpoint(endpoint_descriptor_writer_t *writer, endpoint_description_t *endpoint) {
    celix_status_t status = CELIX_SUCCESS;

    if (endpoint == NULL || writer == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        xmlTextWriterStartElement(writer->writer, ENDPOINT_DESCRIPTION);

        CELIX_PROPERTIES_ITERATE(endpoint->properties, iter) {
			const xmlChar* propertyValue = (const xmlChar*) celix_properties_get(endpoint->properties, iter.key, "");
            xmlTextWriterStartElement(writer->writer, PROPERTY);
            xmlTextWriterWriteAttribute(writer->writer, NAME, (const xmlChar*)iter.key);

            if (strcmp(CELIX_FRAMEWORK_SERVICE_NAME, (char*) iter.key) == 0) {
            	// objectClass *must* be represented as array of string values...
            	endpointDescriptorWriter_writeArrayValue(writer->writer, propertyValue);
            } else if (strcmp(CELIX_RSA_ENDPOINT_SERVICE_ID, (char*) iter.key) == 0) {
            	// endpoint.service.id *must* be represented as long value...
            	endpointDescriptorWriter_writeTypedValue(writer->writer, VALUE_TYPE_LONG, propertyValue);
            } else {
            	// represent all other values as plain string values...
            	endpointDescriptorWriter_writeUntypedValue(writer->writer, propertyValue);
            }

            xmlTextWriterEndElement(writer->writer);
        }

        xmlTextWriterEndElement(writer->writer);
    }

    return status;
}


static char* valueTypeToString(valueType type) {
	switch (type) {
		case VALUE_TYPE_BOOLEAN:
			return "boolean";
		case VALUE_TYPE_BYTE:
			return "byte";
		case VALUE_TYPE_CHAR:
			return "char";
		case VALUE_TYPE_DOUBLE:
			return "double";
		case VALUE_TYPE_FLOAT:
			return "float";
		case VALUE_TYPE_INTEGER:
			return "int";
		case VALUE_TYPE_LONG:
			return "long";
		case VALUE_TYPE_SHORT:
			return "short";
		case VALUE_TYPE_STRING:
			// FALL-THROUGH!
		default:
			return "string";
	}
}
