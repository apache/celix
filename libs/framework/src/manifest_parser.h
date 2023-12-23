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
 * manifest_parser.h
 *
 *  \date       Jul 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MANIFEST_PARSER_H_
#define MANIFEST_PARSER_H_

#include "module.h"
#include "version.h"
#include "manifest.h"

typedef struct manifestParser * manifest_parser_pt;

celix_status_t manifestParser_create(module_pt owner, manifest_pt manifest, manifest_parser_pt *manifest_parser);
celix_status_t manifestParser_destroy(manifest_parser_pt mp);

celix_status_t manifestParser_getAndDuplicateSymbolicName(manifest_parser_pt parser, char **symbolicName);
celix_status_t manifestParser_getAndDuplicateName(manifest_parser_pt parser, char **name);
celix_status_t manifestParser_getAndDuplicateDescription(manifest_parser_pt parser, char **description);
celix_status_t manifestParser_getBundleVersion(manifest_parser_pt parser, version_pt *version);
celix_status_t manifestParser_getAndDuplicateGroup(manifest_parser_pt parser, char **group);

#endif /* MANIFEST_PARSER_H_ */
