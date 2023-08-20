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

use ::celix::BundleContext;
use celix_log_helper_t;
use celix_logHelper_create;
use celix_logHelper_destroy;
use celix_logHelper_log;

#[warn(unused_imports)]
pub enum LogLevel {
    Trace = ::bindings::celix_log_level_CELIX_LOG_LEVEL_TRACE as isize,
    Debug = ::bindings::celix_log_level_CELIX_LOG_LEVEL_DEBUG as isize,
    Info = ::bindings::celix_log_level_CELIX_LOG_LEVEL_INFO as isize,
    Warn = ::bindings::celix_log_level_CELIX_LOG_LEVEL_WARNING as isize,
    Error = ::bindings::celix_log_level_CELIX_LOG_LEVEL_ERROR as isize,
    Fatal = ::bindings::celix_log_level_CELIX_LOG_LEVEL_FATAL as isize,
}

pub struct LogHelper {
    celix_log_helper: *mut celix_log_helper_t,
}

impl LogHelper {
    pub fn new(ctx: &dyn BundleContext, name: &str) -> Self {
        LogHelper {
            celix_log_helper: unsafe { celix_logHelper_create(ctx.get_c_bundle_context(), name.as_ptr() as *const i8) },
        }
    }

    pub fn log(&self, level: LogLevel, message: &str) {
        unsafe {
            celix_logHelper_log(self.celix_log_helper, level as u32, message.as_ptr() as *const i8);
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

    pub fn warn(&self, message: &str) {
        self.log(LogLevel::Warn, message);
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
