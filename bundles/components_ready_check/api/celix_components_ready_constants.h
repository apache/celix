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

#ifndef CELIX_COMPONENTS_READY_CONSTANTS_H_
#define CELIX_COMPONENTS_READY_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief The unique identifier for the components.ready condition.
 *
 * The components ready condition is registered by the framework if the framework.ready condition is registered
 * and all components active.
 */
#define CELIX_CONDITION_ID_COMPONENTS_READY "components.ready"

#ifdef __cplusplus
}
#endif

#endif /* CELIX_COMPONENTS_READY_CONSTANTS_H_ */
