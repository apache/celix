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
 * resolver.h
 *
 *  \date       Jul 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef RESOLVER_H_
#define RESOLVER_H_

#include "module.h"
#include "wire.h"
#include "hash_map.h"

struct importer_wires {
    module_pt importer;
    linked_list_pt wires;
};
typedef struct importer_wires *importer_wires_pt;

linked_list_pt resolver_resolve(module_pt root);
void resolver_moduleResolved(module_pt module);
void resolver_addModule(module_pt module);
void resolver_removeModule(module_pt module);

#endif /* RESOLVER_H_ */
