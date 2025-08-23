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

use std::sync::{Arc, Mutex, RwLock};

use super::BundleContext;
use super::LogLevel;

use celix_bindings::celix_log_service_t;
use crate::ServiceTracker;

pub struct LogHelper {
    name: String,
    tracker: Mutex<Option<ServiceTracker<celix_log_service_t>>>,
    log_svc: RwLock<Option<*const celix_log_service_t>>,
}

impl LogHelper {

    pub fn new(ctx: Arc<BundleContext>, name: &str) -> Arc<Self> {
        let helper = Arc::new(LogHelper{
            name: name.to_string(),
            tracker: Mutex::new(None),
            log_svc: RwLock::new(None),
        });
        let filter = format!("(name={})", name);
        let weak_helper = Arc::downgrade(&helper);
        let tracker = ctx.track_services::<celix_log_service_t>()
            .with_service_name("celix_log_service")
            .with_filter(filter.as_str())
            .with_set_callback(Box::new(move|optional_svc| {
                if let Some(helper) = weak_helper.upgrade() {
                    helper.set_celix_log_service(optional_svc);
                }
            }))
            .build().unwrap();
        helper.tracker.lock().unwrap().replace(tracker);
        helper
    }

    pub fn get_name(&self) -> &str {
        &self.name
    }

    pub fn set_celix_log_service(&self, optional_svc: Option<&celix_log_service_t>) {
        match optional_svc {
            Some(svc) => {
                let svc_ptr: *const celix_log_service_t = svc as *const celix_log_service_t;
                self.log_svc.write().unwrap().replace(svc_ptr);
            }
            None => {
                self.log_svc.write().unwrap().take();
            }
        }
    }

    pub fn log(&self, level: LogLevel, message: &str) {
        let str_result = std::ffi::CString::new(message).unwrap();
        let guard = self.log_svc.read().unwrap();
        if let Some(svc) = guard.as_ref() {
            unsafe {
                if svc.is_null() {
                    return;
                }
                let svc = &**svc;
                if svc.log.is_none() {
                    return;
                }
                let log_fn = svc.log.as_ref().unwrap();
                log_fn(svc.handle, level.into(), str_result.as_ptr());
            }
        }
    }
    pub fn log_trace(&self, message: &str) {
        self.log(LogLevel::Trace, message);
    }

    pub fn log_debug(&self, message: &str) {
        self.log(LogLevel::Debug, message);
    }

    pub fn log_info(&self, message: &str) {
        self.log(LogLevel::Info, message);
    }

    pub fn log_warning(&self, message: &str) {
        self.log(LogLevel::Warning, message);
    }

    pub fn log_error(&self, message: &str) {
        self.log(LogLevel::Error, message);
    }

    pub fn log_fatal(&self, message: &str) {
        self.log(LogLevel::Fatal, message);
    }
}
