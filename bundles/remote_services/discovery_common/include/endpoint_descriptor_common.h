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
 * endpoint_descriptor_common.h
 *
 * \date		Aug 8, 2014
 * \author		<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */

#ifndef ENDPOINT_DESCRIPTOR_COMMON_H_
#define ENDPOINT_DESCRIPTOR_COMMON_H_

/*
 * Private constant & enum definitions for endpoint descriptor reader and writer, not needed for normal usage of the reader and writer.
 */

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

static const __attribute__((unused)) xmlChar* XML = (const xmlChar*) "xml";
static const __attribute__((unused)) xmlChar* XMLNS = (const xmlChar*) "http://www.osgi.org/xmlns/rsa/v1.0.0";

static const __attribute__((unused)) xmlChar* ENDPOINT_DESCRIPTIONS = (const xmlChar*) "endpoint-descriptions";
static const xmlChar* ENDPOINT_DESCRIPTION = (const xmlChar*) "endpoint-description";

static const xmlChar* ARRAY = (const xmlChar*) "array";
static const __attribute__((unused)) xmlChar* LIST = (const xmlChar*) "list";
static const __attribute__((unused)) xmlChar* SET = (const xmlChar*) "set";

static const xmlChar* PROPERTY = (const xmlChar*) "property";
static const xmlChar* NAME = (const xmlChar*) "name";
static const xmlChar* VALUE = (const xmlChar*) "value";
static const xmlChar* VALUE_TYPE = (const xmlChar*) "value-type";

#endif /* ENDPOINT_DESCRIPTOR_COMMON_H_ */
