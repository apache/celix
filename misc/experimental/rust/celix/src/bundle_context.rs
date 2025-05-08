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

use std::any::type_name;
use std::any::Any;
use std::collections::HashMap;
use std::ffi::c_void;
use std::ops::Deref;
use std::ptr::null_mut;
use std::sync::Arc;
use std::sync::Mutex;
use std::sync::Weak;

use celix_bindings::{celix_bundleContext_log, celix_bundleContext_stopTracker, celix_bundleContext_stopTrackerAsync, celix_bundleContext_trackServicesWithOptions, celix_bundleContext_trackServicesWithOptionsAsync, celix_properties_t};
use celix_bindings::celix_service_filter_options;
use celix_bindings::celix_bundleContext_registerServiceWithOptionsAsync;
use celix_bindings::celix_bundleContext_unregisterServiceAsync;
use celix_bindings::celix_service_tracking_options_t;
use celix_bindings::celix_service_use_options_t;
use celix_bindings::celix_service_filter_options_t;
use celix_bindings::celix_bundleContext_useServicesWithOptions;
use celix_bindings::celix_bundleContext_useServiceWithOptions;
use celix_bindings::celix_bundleContext_registerServiceWithOptions;
use celix_bindings::celix_bundleContext_unregisterService;
use celix_bindings::celix_bundle_context_t;
use celix_bindings::celix_properties_create;
use celix_bindings::celix_properties_set;
use celix_bindings::celix_service_registration_options_t;

use super::Error;
use super::LogLevel;

pub struct ServiceRegistration {
    service_id: i64,
    service_name: String,
    unregister_async: bool,
    weak_ctx: Weak<BundleContext>,
    svc: Option<Box<dyn Any>>,
    unmanaged_svc: Option<*mut dyn Any>,
}

impl ServiceRegistration {
    pub fn get_service_id(&self) -> i64 {
        self.service_id
    }

    pub fn get_service_name(&self) -> &str {
        self.service_name.as_str()
    }

    pub fn get_service(&self) -> Option<&dyn Any> {
        if let Some(boxed_svc) = self.svc.as_ref() {
            Some(boxed_svc.as_ref())
        } else if let Some(unmanaged_svc) = self.unmanaged_svc {
            unsafe {
                Some(&*unmanaged_svc)
            }
        } else {
            None
        }
    }
}

impl Drop for ServiceRegistration {
    fn drop(&mut self) {
        let ctx = self.weak_ctx.upgrade();
        match ctx {
            Some(ctx) => ctx.unregister_service(self.service_id, self.unregister_async),
            None => println!("Cannot unregister ServiceRegistration: BundleContext is gone"),
        }
    }
}

pub struct ServiceRegistrationBuilder<'a, T: ?Sized + 'static> {
    ctx: &'a BundleContext,
    register_async: bool,
    unregister_async: bool,
    svc: Option<Box<T>>, //note box is needed for stable pointer value
    unmanaged_svc: Option<*mut dyn Any>,
    service_name: String,
    service_version: String,
    service_properties: HashMap<String, String>,
}

impl<T> ServiceRegistrationBuilder<'_, T> {
    fn new(ctx: &BundleContext) -> ServiceRegistrationBuilder<T> {
        ServiceRegistrationBuilder {
            ctx,
            register_async: false,
            unregister_async: false,
            svc: None,
            unmanaged_svc: None,
            service_name: "".to_string(),
            service_version: "".to_string(),
            service_properties: HashMap::new(),
        }
    }

    pub fn with_service_name(&mut self, name: &str) -> &mut Self {
        self.service_name = name.to_string();
        self
    }

    fn with_service_name_if_not_set(&mut self) -> &mut Self {
        if self.service_name.is_empty() {
            self.service_name = type_name::<T>().to_string();
        }
        self
    }

    pub fn with_service(&mut self, svc: T) -> &mut Self {
        self.svc = Some(Box::new(svc));
        self.with_service_name_if_not_set()
    }

    pub fn with_unmanaged_service(&mut self, svc: *mut T) -> &mut Self {
        self.unmanaged_svc = Some(svc);
        self.with_service_name_if_not_set()
    }

    pub fn with_version(&mut self, version: &str) -> &mut Self {
        self.service_version = version.to_string();
        self
    }

    pub fn with_properties(&mut self, properties: HashMap<String, String>) -> &mut Self {
        self.service_properties = properties;
        self
    }

    pub fn with_property(&mut self, key: &str, value: &str) -> &mut Self {
        self.service_properties
            .insert(key.to_string(), value.to_string());
        self
    }

    pub fn with_register_async(&mut self) -> &mut Self {
        self.register_async = true;
        self
    }

    pub fn with_register_sync(&mut self) -> &mut Self {
        self.register_async = false;
        self
    }

    pub fn with_unregister_async(&mut self) -> &mut Self {
        self.unregister_async = true;
        self
    }

    pub fn with_unregister_sync(&mut self) -> &mut Self {
        self.unregister_async = false;
        self
    }

    fn validate(&self) -> Result<(), Error> {
        let mut valid = true;
        if self.service_name.is_empty() {
            self.ctx
                .log_error("Cannot register service. Service name is empty");
            valid = false;
        }
        if self.svc.is_none() && self.unmanaged_svc.is_none() {
            self.ctx
                .log_error("Cannot register service. No instance provided");
            valid = false;
        }
        match valid {
            true => Ok(()),
            false => Err(Error::BundleException),
        }
    }

    fn get_c_svc(svc_reg: &ServiceRegistration) -> *mut c_void {
        if let Some(boxed_svc) = svc_reg.svc.as_ref() {
            boxed_svc.deref() as *const dyn Any as *mut c_void
        } else if let Some(unmanaged_svc) = svc_reg.unmanaged_svc {
            unmanaged_svc as *mut c_void
        } else {
            null_mut()
        }
    }
    
    pub fn build(&mut self) -> Result<ServiceRegistration, Error> {
        self.validate()?;
        
        let mut svc_reg = ServiceRegistration {
            service_id: -1,
            service_name: self.service_name.clone(),
            unregister_async: self.unregister_async,
            weak_ctx: self.ctx.get_self().clone(),
            svc: if self.svc.is_none() { None } else { Some(self.svc.take().unwrap()) },
            unmanaged_svc: self.unmanaged_svc,
        };
        
        let svc_ptr = Self::get_c_svc(&svc_reg);
        let c_service_name = std::ffi::CString::new(self.service_name.as_str()).unwrap();
        let c_service_version = std::ffi::CString::new(self.service_version.as_str()).unwrap();
        let c_service_properties: *mut celix_properties_t;
        unsafe {
            c_service_properties = celix_properties_create();
            for (key, value) in self.service_properties.iter() {
                let c_key = std::ffi::CString::new(key.as_str()).unwrap();
                let c_value = std::ffi::CString::new(value.as_str()).unwrap();
                celix_properties_set(c_service_properties, c_key.as_ptr(), c_value.as_ptr());
            }
        }

        let opts = celix_service_registration_options_t {
            svc: svc_ptr,
            factory: null_mut(),
            serviceName: c_service_name.as_ptr() as *const i8,
            properties: c_service_properties,
            serviceLanguage: null_mut(),
            serviceVersion: if self.service_version.is_empty() {
                null_mut()
            } else {
                c_service_version.as_ptr() as *const i8
            },
            asyncData: null_mut(),
            asyncCallback: None,
        };

        if self.register_async {
            unsafe {
                svc_reg.service_id = celix_bundleContext_registerServiceWithOptions(self.ctx.get_c_bundle_context(), &opts);
            }
        } else {
            unsafe {
                svc_reg.service_id = celix_bundleContext_registerServiceWithOptionsAsync(self.ctx.get_c_bundle_context(), &opts);
            }
        }
        if svc_reg.service_id >= 0 {
            Ok(svc_reg)
        } else {
            Err(Error::BundleException)
        }
    }
}

pub struct ServiceUseBuilder<'a, T> {
    ctx: &'a BundleContext,
    many: bool,
    service_name: String,
    filter: String,
    callback: Option<Box<dyn Fn(&T)>>, //note double boxed
}

impl<T> ServiceUseBuilder<'_, T> {
    fn new(ctx: &BundleContext, many: bool) -> ServiceUseBuilder<T> {
        ServiceUseBuilder {
            ctx,
            many,
            service_name: type_name::<T>().to_string(),
            filter: "".to_string(),
            callback: None,
        }
    }

    pub fn with_callback(&mut self, closure: Box<dyn Fn(&T)>)-> &mut Self {
        self.callback = Some(closure);
        self
    }

    pub fn with_service_name(&mut self, name: &str) -> &mut Self {
        self.service_name = name.to_string();
        self
    }

    pub fn with_filter(&mut self, filter: &str) -> &mut Self {
        self.filter = filter.to_string();
        self
    }

    fn validate(&self) -> Result<(), Error> {
        if self.service_name.is_empty() {
            return Err(Error::BundleException);
        }
        Ok(())
    }

    unsafe extern "C" fn use_service_c_callback(handle: *mut c_void, svc: *mut c_void) {
        let closure = handle as *const Box<dyn Fn(&T)>;
        let closure = closure.as_ref().unwrap();

        let typed_svc = svc as *const T;
        let typed_svc = typed_svc.as_ref().unwrap();

        closure(typed_svc);
    }

    pub fn build(&mut self) ->  Result<isize, Error> {
        self.validate()?;

        let c_service_name = std::ffi::CString::new(self.service_name.as_str()).unwrap();
        let c_filter = std::ffi::CString::new(self.filter.as_str()).unwrap();
        let c_service_name_ptr: *const i8 = c_service_name.as_ptr();
        let c_filter_ptr: *const i8 = if self.filter.is_empty() { null_mut()} else {c_filter.as_ptr() };

        let c_closure_ptr = self.callback.as_ref().unwrap() as *const Box<dyn Fn(&T)> as *mut c_void;

        let opts = celix_service_use_options_t {
            filter: celix_service_filter_options_t {
                serviceName: c_service_name_ptr,
                versionRange: null_mut(),
                filter: c_filter_ptr,
                serviceLanguage: null_mut(),
                ignoreServiceLanguage: false,
            },
            waitTimeoutInSeconds: 0.0,
            callbackHandle: c_closure_ptr,
            use_: Some(Self::use_service_c_callback),
            useWithProperties: None,
            useWithOwner: None,
            flags: 0,
        };

        if self.many {
            unsafe {
                let count = celix_bundleContext_useServicesWithOptions(self.ctx.get_c_bundle_context(), &opts);
                Ok(count as isize)
            }
        } else {
            unsafe {
                let called = celix_bundleContext_useServiceWithOptions(self.ctx.get_c_bundle_context(), &opts);
                if called {
                    Ok(1)
                } else {
                    Ok(0)
                }
            }
        }
    }
}

struct ServiceTrackerCallbacks<T> {
    set_callback: Option<Box<dyn Fn(Option<&T>)>>,
    add_callback: Option<Box<dyn Fn(&T)>>,
    remove_callback: Option<Box<dyn Fn(&T)>>,
}

pub struct ServiceTracker<T> {
    ctx: Arc<BundleContext>,
    tracker_id: i64,
    // shared_data: Mutex<SharedServiceTrackerData<T>>,
    // data_condition: Condvar,
    callbacks: Box<ServiceTrackerCallbacks<T>>, //Note in a box to ensure pointer value is stable after move
    stop_async: bool,
}

impl<T> ServiceTracker<T> {
    pub fn close(&mut self) {
        self.ctx.stop_tracker(self.tracker_id, self.stop_async);
        self.tracker_id = -1;
    }
}

impl<T> Drop for ServiceTracker<T> {
    fn drop(&mut self) {
        self.close();
    }
}

pub struct ServiceTrackerBuilder<'a, T> {
    ctx: &'a BundleContext,
    service_name: String,
    filter: String,
    track_async: bool,
    stop_async: bool,
    set_callback: Option<Box<dyn Fn(Option<&T>)>>,
    add_callback: Option<Box<dyn Fn(&T)>>,
    remove_callback: Option<Box<dyn Fn(&T)>>,
}

impl<T> ServiceTrackerBuilder<'_, T> {
    fn new(ctx: &BundleContext) -> ServiceTrackerBuilder<T> {
        ServiceTrackerBuilder {
            ctx,
            service_name: type_name::<T>().to_string(),
            filter: "".to_string(),
            track_async: false,
            stop_async: false,
            set_callback: None,
            add_callback: None,
            remove_callback: None,
        }
    }

    pub fn with_service_name(&mut self, name: &str) -> &mut Self {
        self.service_name = name.to_string();
        self
    }

    pub fn with_filter(&mut self, filter: &str) -> &mut Self {
        self.filter = filter.to_string();
        self
    }

    pub fn with_set_callback(&mut self, closure: Box<dyn Fn(Option<&T>)>) -> &mut Self {
        self.set_callback = Some(closure);
        self
    }

    pub fn with_add_callback(&mut self, closure: Box<dyn Fn(&T)>) -> &mut Self {
        self.add_callback = Some(closure);
        self
    }

    pub fn with_remove_callback(&mut self, closure: Box<dyn Fn(&T)>) -> &mut Self {
        self.remove_callback = Some(closure);
        self
    }

    pub fn with_track_async(&mut self) -> &mut Self {
        self.track_async = true;
        self
    }

    pub fn with_track_sync(&mut self) -> &mut Self {
        self.track_async = false;
        self
    }

    pub fn with_stop_async(&mut self) -> &mut Self {
        self.stop_async = true;
        self
    }

    pub fn with_stop_sync(&mut self) -> &mut Self {
        self.stop_async = false;
        self
    }

    fn validate(&self) -> Result<(), Error> {
        if self.service_name.is_empty() {
            return Err(Error::BundleException);
        }
        Ok(())
    }

    unsafe extern "C" fn set_callback_for_c(handle: *mut ::std::os::raw::c_void, svc: *mut ::std::os::raw::c_void) {
        let callbacks = handle as *const ServiceTrackerCallbacks<T>;
        let callbacks = callbacks.as_ref().unwrap();

        if svc.is_null() {
            if let Some(set_callback) = callbacks.set_callback.as_ref() {
                set_callback(None);
            }
        } else {
            let typed_svc = svc as *const T;
            let typed_svc = typed_svc.as_ref().unwrap();
            if let Some(set_callback) = callbacks.set_callback.as_ref() {
                set_callback(Some(typed_svc));
            }
        }
    }

    unsafe extern "C" fn add_callback_for_c(handle: *mut ::std::os::raw::c_void, svc: *mut ::std::os::raw::c_void) {
        let callbacks = handle as *const ServiceTrackerCallbacks<T>;
        let callbacks = callbacks.as_ref().unwrap();

        let typed_svc = svc as *const T;
        let typed_svc = typed_svc.as_ref().unwrap();

        if let Some(add_callback) = callbacks.add_callback.as_ref() {
            add_callback(typed_svc);
        }
    }

    unsafe extern "C" fn remove_callback_for_c(handle: *mut ::std::os::raw::c_void, svc: *mut ::std::os::raw::c_void) {
        let callbacks = handle as *const ServiceTrackerCallbacks<T>;
        let callbacks = callbacks.as_ref().unwrap();

        let typed_svc = svc as *const T;
        let typed_svc = typed_svc.as_ref().unwrap();

        if let Some(remove_callback) = callbacks.remove_callback.as_ref() {
            remove_callback(typed_svc);
        }
    }

    pub fn build(&mut self) -> Result<ServiceTracker<T>, Error> {
        self.validate()?;

        let mut svc_tracker = ServiceTracker {
            ctx: self.ctx.get_self().upgrade().unwrap(),
            tracker_id: -1,
            callbacks: Box::new(ServiceTrackerCallbacks {
                set_callback: self.set_callback.take(),
                add_callback: self.add_callback.take(),
                remove_callback: self.remove_callback.take(),
            }),
            stop_async: self.stop_async,
        };

        let c_service_name = std::ffi::CString::new(self.service_name.as_str()).unwrap();
        let c_filter = std::ffi::CString::new(self.filter.as_str()).unwrap();
        let c_callback_handle = svc_tracker.callbacks.as_ref() as *const ServiceTrackerCallbacks<T> as *mut c_void;

        let opts = celix_service_tracking_options_t{
            filter: celix_service_filter_options {
                serviceName: c_service_name.as_ptr(),
                versionRange: null_mut(),
                filter: if self.filter.is_empty() { null_mut() } else { c_filter.as_ptr() },
                serviceLanguage: null_mut(),
                ignoreServiceLanguage: false,
            },
            callbackHandle: c_callback_handle,
            set: Some(Self::set_callback_for_c),
            setWithProperties: None,
            setWithOwner: None,
            add: Some(Self::add_callback_for_c),
            addWithProperties: None,
            addWithOwner: None,
            remove: Some(Self::remove_callback_for_c),
            removeWithProperties: None,
            removeWithOwner: None,
            trackerCreatedCallbackData: null_mut(),
            trackerCreatedCallback: None,
        };

        let svc_tracker_id: i64;
        unsafe {
            if self.track_async {
                svc_tracker_id = celix_bundleContext_trackServicesWithOptionsAsync(self.ctx.get_c_bundle_context(), &opts);
            } else {
                svc_tracker_id = celix_bundleContext_trackServicesWithOptions(self.ctx.get_c_bundle_context(), &opts);
            }
        }

        if svc_tracker_id >= 0 {
            svc_tracker.tracker_id = svc_tracker_id;
            Ok(svc_tracker)
        } else {
            Err(Error::BundleException)
        }
    }
}

pub struct BundleContext {
    c_bundle_context: *mut celix_bundle_context_t,
    weak_self: Mutex<Option<Weak<BundleContext>>>,
}

impl BundleContext {
    fn new(c_bundle_context: *mut celix_bundle_context_t) -> Arc<Self> {
        let ctx = Arc::new(BundleContext {
            c_bundle_context,
            weak_self: Mutex::new(None),
        });
        let weak_ref = Arc::downgrade(&ctx);
        ctx.set_self(weak_ref);
        ctx
    }

    fn set_self(&self, weak_self: Weak<BundleContext>) {
        let mut guard = self.weak_self.lock().unwrap();
        *guard = Some(weak_self);
    }

    fn get_self(&self) -> Weak<BundleContext> {
        self.weak_self.lock().unwrap().clone().unwrap()
    }

    fn log_to_c(&self, level: LogLevel, message: &str) {
        unsafe {
            let result = std::ffi::CString::new(message);
            match result {
                Ok(c_str) => {
                    celix_bundleContext_log(
                        self.c_bundle_context,
                        level.into(),
                        c_str.as_ptr() as *const i8,
                    );
                }
                Err(e) => {
                    println!("Error creating CString: {}", e);
                }
            }
        }
    }

    pub fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t {
        self.c_bundle_context
    }

    pub fn log(&self, level: LogLevel, message: &str) {
        self.log_to_c(level, message);
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

    pub fn register_service<T>(&self) -> ServiceRegistrationBuilder<T> {
        ServiceRegistrationBuilder::new(self)
    }

    fn unregister_service(&self, service_id: i64, unregister_async: bool) {
        unsafe {
            if unregister_async {
                celix_bundleContext_unregisterServiceAsync(self.c_bundle_context, service_id, null_mut(), None);
            } else {
                celix_bundleContext_unregisterService(self.c_bundle_context, service_id);
            }
        }
    }

    pub fn use_service<T>(&self) -> ServiceUseBuilder<T> {
        ServiceUseBuilder::new(self, false)
    }

    pub fn use_services<T>(&self) -> ServiceUseBuilder<T> {
        ServiceUseBuilder::new(self, true)
    }

    pub fn track_services<T>(&self) -> ServiceTrackerBuilder<T> { ServiceTrackerBuilder::new(self) }

    fn stop_tracker(&self, tracker_id: i64, stop_async: bool) {
        unsafe {
            if stop_async {
                celix_bundleContext_stopTrackerAsync(self.c_bundle_context, tracker_id, null_mut(), None);
            } else {
                celix_bundleContext_stopTracker(self.c_bundle_context, tracker_id);
            }
        }
    }
}

pub fn bundle_context_new(c_bundle_context: *mut celix_bundle_context_t) -> Arc<BundleContext> {
    BundleContext::new(c_bundle_context)
}
