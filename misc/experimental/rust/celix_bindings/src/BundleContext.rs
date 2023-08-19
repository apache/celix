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
use celix_log_level_CELIX_LOG_LEVEL_INFO;

pub trait BundleContext {
    fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t;

    fn log_info(&self, message: &str);
}

//note BundleContextImpl is pub for usage in bundle activator macro. TODO check if this can be avoided
pub struct BundleContextImpl {
    c_bundle_context: *mut celix_bundle_context_t,
}

impl BundleContextImpl {
    pub fn new(c_bundle_context: *mut celix_bundle_context_t) -> Self {
        BundleContextImpl {
            c_bundle_context,
        }
    }
}

impl BundleContext for BundleContextImpl {
    fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t {
        self.c_bundle_context
    }

    fn log_info(&self, message: &str) {
        unsafe {
            celix_bundleContext_log(self.c_bundle_context, celix_log_level_CELIX_LOG_LEVEL_INFO as u32, message.as_ptr() as *const i8);
        }
    }
}
