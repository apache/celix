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
 * manifest_parser.h
 *
 *  Created on: Jul 13, 2010
 *      Author: alexanderb
 */

#ifndef MANIFEST_PARSER_H_
#define MANIFEST_PARSER_H_

#include "module.h"
#include "version.h"
#include "manifest.h"
#include "linkedlist.h"

typedef struct manifestParser * MANIFEST_PARSER;

celix_status_t manifestParser_create(MODULE owner, MANIFEST manifest, apr_pool_t *memory_pool, MANIFEST_PARSER *manifest_parser);

celix_status_t manifestParser_getSymbolicName(MANIFEST_PARSER parser, apr_pool_t *pool, char **symbolicName);
celix_status_t manifestParser_getBundleVersion(MANIFEST_PARSER parser, apr_pool_t *pool, VERSION *version);
celix_status_t manifestParser_getCapabilities(MANIFEST_PARSER parser, apr_pool_t *pool, LINKED_LIST *capabilities);
celix_status_t manifestParser_getRequirements(MANIFEST_PARSER parser, apr_pool_t *pool, LINKED_LIST *requirements);

#endif /* MANIFEST_PARSER_H_ */
