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

#ifndef CELIX_CONDITION_H_
#define CELIX_CONDITION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief The name of the condition service.
 */
#define CELIX_CONDITION_SERVICE_NAME "celix_condition"

/*!
 * @brief The version of the condition service.
 */
#define CELIX_CONDITION_SERVICE_VERSION "1.0.0"

/*!
 * @brief The property key used to specify the condition id.
 * A condition id can only identify a single condition.
 */
#define CELIX_CONDITION_ID "condition.id"

/**
 * @brief Celix condition service struct.
 *
 * In dynamic systems, such as OSGi and Apache Celix, one of the more challenging problems can be to define when
 * a system or part of it is ready to do work.
 *
 * The celix_condition service interface is a marker interface designed to address this issue.
 * Its role is to provide a dependency that can be tracked. It acts as a defined signal to other services.
 *
 * A celix_condition service must be registered with the "condition.id" service property.
 */
typedef struct celix_condition {
    void* handle; /**< private dummy handle, note not used in marker service struct, but added to ensure
                     sizeof(celix_condition_t) != 0  */
} celix_condition_t;

/*!
 * @brief The unique identifier for the default framework true condition.
 * The default True condition is registered by the framework during framework initialization and therefore
 * can always be relied upon.
 */
#define CELIX_CONDITION_ID_TRUE "true"

/*!
 * @brief The unique identifier for the framework.ready condition.
 * The framework ready condition is registered by the framework after all configured bundles are installed - and
 * if configured - started and after the Apache Celix Event Queue becomes empty.
 *
 * Note that after the framework ready condition is registered, the event queue can become non-empty again.
 * For example if a component depends on the "framework.ready" condition.
 *
 * Either a framework.ready or framework.error condition is registered.
 */
#define CELIX_CONDITION_ID_FRAMEWORK_READY "framework.ready"

/*!
 * @brief The unique identifier for the framework.error condition.

 * The framework error condition is registered by the framework after all configured bundles are processed,
 * but an error occurred while installing or starting a bundle.
 *
 * Either a framework.ready or framework.error condition is registered.
 */
#define CELIX_CONDITION_ID_FRAMEWORK_ERROR "framework.error"

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

#endif /* CELIX_CONDITION_H_ */
