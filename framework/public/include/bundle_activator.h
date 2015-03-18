/*
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
/**
 *
 * @defgroup BundleActivator BundleActivator
 * @ingroup framework
 * @{
 *	\brief		Customizes the starting and stopping of a bundle.
 *	\details	\ref BundleActivator is a header that must be implemented by every
 * 				bundle. The Framework creates/starts/stops/destroys activator instances using the
 * 				functions described in this header. If the bundleActivator_start()
 * 				function executes successfully, it is guaranteed that the same instance's
 * 				bundleActivator_stop() function will be called when the bundle is
 * 				to be stopped. The same applies to the bundleActivator_create() and
 * 				bundleActivator_destroy() functions.
 * 				The Framework must not concurrently call the activator functions.
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \date      	March 18, 2010
 *  \copyright	Apache License, Version 2.0
 */
#ifndef BUNDLE_ACTIVATOR_H_
#define BUNDLE_ACTIVATOR_H_

#include "bundle_context.h"
#include "framework_exports.h"

/**
 * Called when this bundle is started so the bundle can create an instance for its activator.
 * The framework does not assume any type for the activator instance, this is implementation specific.
 * The activator instance is handle as a void pointer by the framework, the implementation must cast it to the
 * implementation specific type.
 *
 * @param context The execution context of the bundle being started.
 * @param[out] userData A pointer to the specific activator instance used by this bundle.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
ACTIVATOR_EXPORT celix_status_t bundleActivator_create(bundle_context_pt context_ptr, void **userData);

/**
 * Called when this bundle is started so the Framework can perform the bundle-specific activities necessary
 * to start this bundle. This method can be used to register services or to allocate any resources that this
 * bundle needs.
 *
 * <p>
 * This method must complete and return to its caller in a timely manner.
 *
 * @param userData The activator instance to be used.
 * @param context The execution context of the bundle being started.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
ACTIVATOR_EXPORT celix_status_t bundleActivator_start(void *userData, bundle_context_pt context);

/**
 * Called when this bundle is stopped so the Framework can perform the bundle-specific activities necessary
 * to stop the bundle. In general, this method should undo the work that the <code>bundleActivator_start()</code>
 * function started. There should be no active threads that were started by this bundle when this bundle returns.
 * A stopped bundle must not call any Framework objects.
 *
 * <p>
 * This method must complete and return to its caller in a timely manner.
 *
 * @param userData The activator instance to be used.
 * @param context The execution context of the bundle being stopped.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
ACTIVATOR_EXPORT celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context);

/**
 * Called when this bundle is stopped so the bundle can destroy the instance of its activator. In general, this
 * method should undo the work that the <code>bundleActivator_create()</code> function initialized.
 *
 * <p>
 * This method must complete and return to its caller in a timely manner.
 *
 * @param userData The activator instance to be used.
 * @param context The execution context of the bundle being stopped.
 *
 * @return Status code indication failure or success:
 * 		- CELIX_SUCCESS when no errors are encountered.
 * 		- Any other status code will mark the bundle as stopped and the framework will remove this
 * 		  bundle's listeners, unregister all services, and release all services used by this bundle.
 */
ACTIVATOR_EXPORT celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt  __attribute__((unused))  __attribute__((unused)) context);

#endif /* BUNDLE_ACTIVATOR_H_ */

/**
 * @}
 */
