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

#ifndef CELIX_CELIX_LAUNCHER_PRIVATE_H
#define CELIX_CELIX_LAUNCHER_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stop the framework, by stopping the framework bundle.
 *
 * Also logs a message if the framework is stopped due to a signal.
 *
 * @param signal The optional signal that caused the framework to stop.
 */
void celix_launcher_stopInternal(const int* signal);

/**
 * @brief Create a combined configuration properties set by combining the embedded properties with the runtime
 * properties.
 * @param[in,out] embeddedProps The embedded properties to use and extend with the runtime properties.
 * @param[in] runtimeProps The optional runtime properties file to load and use.
 */
celix_status_t celix_launcher_combineProperties(celix_properties_t* embeddedProps,
                                                const celix_properties_t* runtimeProps);

#ifdef __cplusplus
}
#endif

#endif // CELIX_CELIX_LAUNCHER_PRIVATE_H
