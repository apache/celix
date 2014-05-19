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
 * resolver_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "resolver.h"

linked_list_pt resolver_resolve(module_pt root) {
	mock_c()->actualCall("resolver_resolve")
			->withPointerParameters("module", root);
	return mock_c()->returnValue().value.pointerValue;
}

void resolver_moduleResolved(module_pt module) {
	mock_c()->actualCall("resolver_moduleResolved")
		->withPointerParameters("module", module);
}

void resolver_addModule(module_pt module) {
	mock_c()->actualCall("resolver_addModule")
		->withPointerParameters("module", module);
}

void resolver_removeModule(module_pt module) {
	mock_c()->actualCall("resolver_removeModule")
		->withPointerParameters("module", module);
}

