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
 * endpoint_description_writer.c
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <libxml/xmlwriter.h>

#include "endpoint_description.h"
#include "endpoint_descriptor_writer.h"

struct endpoint_descriptor_writer {
    xmlBufferPtr buffer;
    xmlTextWriterPtr writer;
};

static celix_status_t endpointDescriptorWriter_writeEndpoint(endpoint_descriptor_writer_pt writer, endpoint_description_pt endpoint);

celix_status_t endpointDescriptorWriter_create(endpoint_descriptor_writer_pt *writer) {
    celix_status_t status = CELIX_SUCCESS;

    *writer = malloc(sizeof(**writer));
    if (!writer) {
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

celix_status_t endpointDescriptorWriter_destroy(endpoint_descriptor_writer_pt writer) {
    xmlFreeTextWriter(writer->writer);
    xmlBufferFree(writer->buffer);
    free(writer);
    return CELIX_SUCCESS;
}

celix_status_t endpointDescriptorWriter_writeDocument(endpoint_descriptor_writer_pt writer, array_list_pt endpoints, char **document) {
    celix_status_t status = CELIX_SUCCESS;
    int rc;

    rc = xmlTextWriterStartDocument(writer->writer, NULL, "UTF-8", NULL);
    if (rc < 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        rc = xmlTextWriterStartElementNS(writer->writer, NULL, BAD_CAST "endpoint-descriptions", BAD_CAST "http://www.osgi.org/xmlns/rsa/v1.0.0");
        if (rc < 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        } else {
            int i;
            for (i = 0; i < arrayList_size(endpoints); i++) {
                endpoint_description_pt endpoint = arrayList_get(endpoints, i);
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

static celix_status_t endpointDescriptorWriter_writeEndpoint(endpoint_descriptor_writer_pt writer, endpoint_description_pt endpoint) {
    celix_status_t status = CELIX_SUCCESS;

    if (endpoint == NULL || writer == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        xmlTextWriterStartElement(writer->writer, BAD_CAST "endpoint-description");

        hash_map_iterator_pt iter = hashMapIterator_create(endpoint->properties);
        while (hashMapIterator_hasNext(iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
            xmlTextWriterStartElement(writer->writer, BAD_CAST "property");
            xmlTextWriterWriteAttribute(writer->writer, BAD_CAST "name", hashMapEntry_getKey(entry));
            xmlTextWriterWriteAttribute(writer->writer, BAD_CAST "value", hashMapEntry_getValue(entry));
            xmlTextWriterEndElement(writer->writer);
        }
        hashMapIterator_destroy(iter);

        xmlTextWriterEndElement(writer->writer);
    }

    return status;
}


int main() {
    endpoint_descriptor_writer_pt writer = NULL;
    endpointDescriptorWriter_create(&writer);
    array_list_pt list = NULL;
    arrayList_create(&list);

    properties_pt props = properties_create();
    properties_set(props, "a", "b");
    endpoint_description_pt epd = NULL;
    endpointDescription_create(props, &epd);
    arrayList_add(list, epd);

    properties_pt props2 = properties_create();
    properties_set(props2, "a", "b");
    endpoint_description_pt epd2 = NULL;
    endpointDescription_create(props2, &epd2);
    arrayList_add(list, epd2);

    char *buffer = NULL;
    endpointDescriptorWriter_writeDocument(writer, list, &buffer);

    arrayList_destroy(list);
    endpointDescription_destroy(epd);
    endpointDescription_destroy(epd2);
    endpointDescriptorWriter_destroy(writer);

    printf("%s\n", buffer);
}
