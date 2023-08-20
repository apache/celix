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

use super::LogLevel;

use celix_bindings::celix_bundle_context_t;
use celix_bindings::celix_bundleContext_log;
use celix_bindings::celix_log_level_e;

pub trait BundleContext {
    fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t;

    fn log(&self, level: LogLevel, message: &str);

    fn log_trace(&self, message: &str);

    fn log_debug(&self, message: &str);

    fn log_info(&self, message: &str);

    fn log_warning(&self, message: &str);

    fn log_error(&self, message: &str);

    fn log_fatal(&self, message: &str);
}

struct BundleContextImpl {
    c_bundle_context: *mut celix_bundle_context_t,
}

impl BundleContextImpl {
    pub fn new(c_bundle_context: *mut celix_bundle_context_t) -> Self {
        BundleContextImpl {
            c_bundle_context,
        }
    }
    fn log_to_c(&self, level: LogLevel, message: &str) {
        unsafe {
            let result = std::ffi::CString::new(message);
            match result {
                Ok(c_str) => {
                    celix_bundleContext_log(self.c_bundle_context, level.into(), c_str.as_ptr() as *const i8);
                }
                Err(e) => {
                    println!("Error creating CString: {}", e);
                }
            }
        }
    }
}

impl BundleContext for BundleContextImpl {
    fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t {
        self.c_bundle_context
    }

    fn log(&self, level: LogLevel, message: &str) { self.log_to_c(level, message); }

    fn log_trace(&self, message: &str) { self.log(LogLevel::Trace, message); }

    fn log_debug(&self, message: &str) { self.log(LogLevel::Debug, message); }

    fn log_info(&self, message: &str) { self.log(LogLevel::Info, message); }

    fn log_warning(&self, message: &str) { self.log(LogLevel::Warning, message); }

    fn log_error(&self, message: &str){ self.log(LogLevel::Error, message); }

    fn log_fatal(&self, message: &str){ self.log(LogLevel::Fatal, message); }
}

pub fn bundle_context_new(c_bundle_context: *mut celix_bundle_context_t) -> Box<dyn BundleContext> {
    Box::new(BundleContextImpl::new(c_bundle_context))
}
