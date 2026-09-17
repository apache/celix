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

// Re-export the celix_status_t and celix_bundle_context_t C API in this crate public API so that
// it can be used in the generate_bundle_activator macro.
// Note that as result the celix rust lib is leaking the celix_status_t and celix_bundle_context_t
// C API in its public API.
#[doc(hidden)]
pub mod details {
    pub use celix_bindings::celix_bundle_context_t as CBundleContext;
    pub use celix_bindings::celix_status_t as CStatus;
}

mod errno;
// Re-export errno types in the public API.
pub use self::errno::Error;
pub use self::errno::CELIX_SUCCESS;

mod log_level;
// Re-export log level types in the public API.
pub use self::log_level::LogLevel;

mod bundle_context;
// Re-export bundle context types in the public API.
pub use self::bundle_context::bundle_context_new;
pub use self::bundle_context::BundleContext;
pub use self::bundle_context::ServiceRegistration;
pub use self ::bundle_context::ServiceTracker;

mod bundle_activator;
// Re-export bundle activator types in the public API.
pub use self::bundle_activator::BundleActivator;

mod log_helper;
// Re-export log helper types in the public API.
pub use self::log_helper::LogHelper;
