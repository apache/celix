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

use std::sync::Arc;

use super::LogLevel;
use super::BundleContext;

use celix_bindings::celix_log_helper_t;
use celix_bindings::celix_logHelper_create;
use celix_bindings::celix_logHelper_destroy;
use celix_bindings::celix_logHelper_log;
pub struct LogHelper {
    celix_log_helper: *mut celix_log_helper_t,
}

impl LogHelper {
    pub fn new(ctx: Arc<dyn BundleContext>, name: &str) -> Self {
        unsafe {
            let result = std::ffi::CString::new(name);
            match result {
                Ok(c_str) => {
                    LogHelper {
                        celix_log_helper: celix_logHelper_create(ctx.get_c_bundle_context(), c_str.as_ptr() as *const i8),
                    }
                }
                Err(e) => {
                    ctx.log_error(&format!("Error creating CString: {}. Using \"error\" as log name", e));
                    let c_str = std::ffi::CString::new("error").unwrap();
                    LogHelper {
                        celix_log_helper: celix_logHelper_create(ctx.get_c_bundle_context(), c_str.as_ptr() as *const i8),
                    }
                }
            }
        }
    }

    pub fn log(&self, level: LogLevel, message: &str) {
        unsafe {
            let result = std::ffi::CString::new(message);
            match result {
                Ok(c_str) => {
                    celix_logHelper_log(self.celix_log_helper, level.into(), c_str.as_ptr() as *const i8);
                }
                Err(e) => {
                    println!("Error creating CString: {}", e);
                }
            }
        }
    }
    pub fn trace(&self, message: &str) {
        self.log(LogLevel::Trace, message);
    }

    pub fn debug(&self, message: &str) {
        self.log(LogLevel::Debug, message);
    }

    pub fn info(&self, message: &str) {
        self.log(LogLevel::Info, message);
    }

    pub fn warning(&self, message: &str) {
        self.log(LogLevel::Warning, message);
    }

    pub fn error(&self, message: &str) {
        self.log(LogLevel::Error, message);
    }

    pub fn fatal(&self, message: &str) {
        self.log(LogLevel::Fatal, message);
    }
}

impl Drop for LogHelper {
    fn drop(&mut self) {
        unsafe {
            celix_logHelper_destroy(self.celix_log_helper);
        }
    }
}
