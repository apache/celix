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
 * xml_parser_impl.h
 *
 *  \date       Nov 3, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#ifndef XML_PARSER_IMPL_H_
#define XML_PARSER_IMPL_H_

#include "component_metadata.h"

typedef struct xml_parser *xml_parser_t;

celix_status_t xmlParser_create(apr_pool_t *pool, xml_parser_t *parser);
celix_status_t xmlParser_parseComponent(xml_parser_t parser, char *componentEntry, component_t *component);

#endif /* XML_PARSER_IMPL_H_ */
