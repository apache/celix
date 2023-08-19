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

#[allow(non_camel_case_types, non_snake_case, non_upper_case_globals, dead_code)]
mod bindings {
     include!(concat!(env!("OUT_DIR"), "/celix_bindings.rs"));
}
pub use bindings::*;

//Note C #defines (compile-time constants) are not all generated in the bindings.
pub const CELIX_SUCCESS: celix_status_t = 0;
pub const CELIX_BUNDLE_EXCEPTION: celix_status_t = 70001;

pub mod celix {
    use celix_status_t;
    use CELIX_SUCCESS;
    use CELIX_BUNDLE_EXCEPTION;

    use celix_bundle_context_t;
    use celix_bundleContext_log;

    use celix_log_helper_t;
    use celix_logHelper_create;
    use celix_logHelper_log;
    use celix_logHelper_destroy;

    include!("BundleContext.rs");
    include!("BundleActivator.rs");
    include!("LogHelper.rs");
}
