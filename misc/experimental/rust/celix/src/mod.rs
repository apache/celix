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

use celix_status_t;

//Note C #defines (compile-time constants) are not all generated in the bindings file.
//So introduce them here as constants.
pub const CELIX_SUCCESS: celix_status_t = 0;

mod errno;
// Export errno types in the public API.
pub use self::errno::Error as Error;

mod bundle_context;
// Export bundle context types in the public API.
pub use self::bundle_context::BundleContext as BundleContext;
pub use self::bundle_context::create_bundle_context_instance as create_bundle_context_instance;

mod bundle_activator;
// Export bundle activator types in the public API.
pub use self::bundle_activator::BundleActivator as BundleActivator;

// mod log_helper;
// // Export log helper types in the public API.
// pub use self::log_helper::LogHelper as LogHelper;
