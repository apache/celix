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

extern crate celix_bindings;

use std::error::Error;
use std::os::raw::c_void;
use std::ffi::CString;
use std::ffi::NulError;
use celix_bindings::*; //Add all Apache Celix C bindings to the namespace (i.e. celix_bundleContext_log, etc.)

struct RustBundle {
    name: String,
    ctx: *mut celix_bundle_context_t,
}

impl RustBundle {

    unsafe fn new(name: String, ctx: *mut celix_bundle_context_t) -> Result<RustBundle, NulError> {
        let result = RustBundle {
            name,
            ctx,
        };
        result.log_lifecycle("created")?;
        Ok(result)
    }

    unsafe fn log_lifecycle(&self, event: &str) -> Result<(), NulError> {
        let id = celix_bundleContext_getBundleId(self.ctx);
        let c_string = CString::new(format!("Rust Bundle '{}' with id {} {}!", self.name, id, event))?;
        celix_bundleContext_log(self.ctx, celix_log_level_CELIX_LOG_LEVEL_INFO, c_string.as_ptr());
        Ok(())
    }

    unsafe fn start(&self) -> Result<(), NulError> {
        self.log_lifecycle("started")
    }

    unsafe fn stop(&self) -> Result<(), NulError> {
        self.log_lifecycle("stopped")
    }
}

impl Drop for RustBundle {
    fn drop(&mut self) {
        unsafe {
            let result = self.log_lifecycle("destroyed");
            match result {
                Ok(()) => (),
                Err(e) => println!("Error while logging: {}", e),
            }
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_create(ctx: *mut celix_bundle_context_t, data: *mut *mut c_void) -> celix_status_t {
    let rust_bundle = RustBundle::new("Hello World".to_string(), ctx);
    if rust_bundle.is_err() {
        return CELIX_BUNDLE_EXCEPTION;
    }
    *data = Box::into_raw(Box::new(rust_bundle.unwrap())) as *mut c_void;
    CELIX_SUCCESS
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_start(data: *mut c_void, _ctx: *mut celix_bundle_context_t) -> celix_status_t {
    let rust_bundle = &*(data as *mut RustBundle);
    let result = rust_bundle.start();
    match result {
        Ok(()) => CELIX_SUCCESS,
        Err(_) => CELIX_BUNDLE_EXCEPTION,
    }
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_stop(data: *mut c_void, _ctx: *mut celix_bundle_context_t) -> celix_status_t {
    let rust_bundle = &*(data as *mut RustBundle);
    let result = rust_bundle.stop();
    match result {
        Ok(()) => CELIX_SUCCESS,
        Err(_) => CELIX_BUNDLE_EXCEPTION,
    }
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_destroy(data: *mut c_void, _ctx: *mut celix_bundle_context_t) -> celix_status_t {
    let rust_bundle = Box::from_raw(data as *mut RustBundle);
    drop(rust_bundle);
    CELIX_SUCCESS
}
