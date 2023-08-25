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
use std::ptr::null_mut;
use std::sync::Arc;
use std::sync::Mutex;
use std::sync::Weak;

use celix_bindings::celix_bundleContext_log;
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
    weak_ctx: Weak<BundleContextImpl>,
    _boxed_svc: Option<Box<dyn Any>>,
}

impl Drop for ServiceRegistration {
    fn drop(&mut self) {
        let ctx = self.weak_ctx.upgrade();
        match ctx {
            Some(ctx) => ctx.unregister_service(self.service_id),
            None => println!("Cannot unregister ServiceRegistration: BundleContext is gone"),
        }
    }
}

pub struct ServiceRegistrationBuilder<'a> {
    ctx: &'a BundleContextImpl,
    boxed_svc: Option<Box<dyn Any>>,
    unmanaged_svc: *mut c_void,
    service_name: String,
    service_version: String,
    service_properties: HashMap<String, String>,
}

impl ServiceRegistrationBuilder<'_> {
    fn new(ctx: &BundleContextImpl) -> ServiceRegistrationBuilder {
        ServiceRegistrationBuilder {
            ctx,
            boxed_svc: None,
            unmanaged_svc: null_mut(),
            service_name: "".to_string(),
            service_version: "".to_string(),
            service_properties: HashMap::new(),
        }
    }

    pub fn with_service_name(&mut self, name: &str) -> &mut Self {
        self.service_name = name.to_string();
        self
    }

    fn with_service_name_if_not_set(&mut self, name: &str) -> &mut Self {
        if self.service_name.is_empty() {
            self.service_name = name.to_string();
        }
        self
    }

    pub fn with_service<I: 'static>(&mut self, svc: I) -> &mut Self {
        self.boxed_svc = Some(Box::new(svc));
        self.unmanaged_svc = null_mut();
        self.with_service_name_if_not_set(type_name::<I>())
    }

    pub fn with_boxed_service<T: ?Sized + 'static>(&mut self, svc: Box<T>) -> &mut Self {
        self.boxed_svc = Some(Box::new(svc));
        self.unmanaged_svc = null_mut();
        self.with_service_name_if_not_set(type_name::<T>())
    }

    pub fn with_unmanaged_service<I>(&mut self, svc: *mut I) -> &mut Self {
        self.boxed_svc = None;
        self.unmanaged_svc = svc as *mut c_void;
        self.with_service_name_if_not_set(type_name::<I>())
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

    fn validate(&self) -> Result<(), Error> {
        let mut valid = true;
        if self.service_name.is_empty() {
            self.ctx
                .log_error("Cannot register service. Service name is empty");
            valid = false;
        }
        if self.boxed_svc.is_none() && /*self.arc_svc.is_none() &&*/ self.unmanaged_svc.is_null() {
            self.ctx
                .log_error("Cannot register service. No instance provided");
            valid = false;
        }
        match valid {
            true => Ok(()),
            false => Err(Error::BundleException),
        }
    }

    fn get_c_svc(&mut self) -> *mut c_void {
        if let Some(boxed_svc) = self.boxed_svc.as_mut() {
            let any_svc: &mut dyn Any = boxed_svc.as_mut();
            let boxed_svc_ptr = any_svc as *mut dyn Any; //note box still owns the instance
            boxed_svc_ptr as *mut c_void
        } else if self.unmanaged_svc.is_null() {
            panic!("Cannot get c_svc. No instance provided");
        } else {
            self.unmanaged_svc
        }
    }

    unsafe fn build_unsafe(&mut self, svc_ptr: *mut c_void) -> Result<ServiceRegistration, Error> {
        let c_service_name = std::ffi::CString::new(self.service_name.as_str()).unwrap();
        let c_service_version = std::ffi::CString::new(self.service_version.as_str()).unwrap();
        let c_service_properties = celix_properties_create();
        for (key, value) in self.service_properties.iter() {
            let c_key = std::ffi::CString::new(key.as_str()).unwrap();
            let c_value = std::ffi::CString::new(value.as_str()).unwrap();
            celix_properties_set(c_service_properties, c_key.as_ptr(), c_value.as_ptr());
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

        let service_id: i64 =
            celix_bundleContext_registerServiceWithOptions(self.ctx.get_c_bundle_context(), &opts);
        if service_id >= 0 {
            Ok(ServiceRegistration {
                service_id,
                weak_ctx: self.ctx.get_self().clone(),
                _boxed_svc: self.boxed_svc.take(), //to ensure that a possible box instance is not dropped
                                                   // arc_svc: self.arc_svc.take(), //to ensure that a possible arc instance is not dropped
            })
        } else {
            Err(Error::BundleException)
        }
    }

    pub fn build(&mut self) -> Result<ServiceRegistration, Error> {
        self.validate()?;
        let svc_ptr = self.get_c_svc();
        unsafe {
            self.build_unsafe(svc_ptr)
        }
    }
}

pub struct ServiceUseBuilder<'a> {
    ctx: &'a BundleContextImpl,
    many: bool,
    service_name: String,
    filter: String,
    callback: Option<Box<Box<dyn Fn(&dyn Any)>>>
}

impl ServiceUseBuilder<'_> {
    fn new(ctx: &BundleContextImpl, many: bool) -> ServiceUseBuilder {
        ServiceUseBuilder {
            ctx,
            many,
            service_name: "".to_string(),
            filter: "".to_string(),
            callback: None,
        }
    }

    pub fn with_service<T: ?Sized + 'static>(&mut self) -> &mut Self {
        self.service_name = type_name::<T>().to_string();
        self
    }

    #[doc = " @brief Provide a callback which will be called when a service is available. T must be a Sized (non trait) type, due to the use of downcast_ref"]
    pub fn with_callback<T: Sized + 'static>(&mut self, closure: Box<dyn Fn(&T)>)-> &mut Self {
        if self.service_name.is_empty() {
            self.with_service::<T>();
        }

        let any_closure = Box::new(move |any_svc: &dyn Any| {
            if let Some(svc) = any_svc.downcast_ref::<T>() {
                closure(svc);
            }
        });

        self.callback = Some(Box::new(any_closure));
        self
    }

    #[doc = " @brief Provide a callback which will be called when a service is available, with a dyn Any argument. Note that this is useful for trait objects."]
    pub fn with_any_callback(&mut self, closure: Box<dyn Fn(&dyn Any)>)-> &mut Self {
        self.callback = Some(Box::new(closure)); //note double boxed
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
        if self.callback.is_none() || self.service_name.is_empty() {
            return Err(Error::BundleException);
        }
        Ok(())
    }

    unsafe extern "C" fn use_service_c_callback(handle: *mut c_void, svc: *mut c_void) {
        let boxed_fn = Box::from_raw(handle as *mut Box<dyn Fn(&dyn Any)>);
        let any_svc_ptr = svc as *mut dyn Any;
        let any_svc_ref = any_svc_ptr.as_ref().unwrap();
        boxed_fn(any_svc_ref);
    }

    pub fn build(&mut self) ->  Result<isize, Error> {
        self.validate()?;

        let c_service_name = std::ffi::CString::new(self.service_name.as_str()).unwrap();
        let c_filter = std::ffi::CString::new(self.filter.as_str()).unwrap();
        let c_service_name_ptr: *const i8 = c_service_name.as_ptr();

        //Note filter is for now unused, introduce when updating to use of celix_bundleContext_useServiceWithOptions
        let c_filter_ptr: *const i8 = if self.filter.is_empty() { null_mut()} else {c_filter.as_ptr() };

        let boxed_fn = self.callback.take().unwrap();
        let fn_ptr = Box::into_raw(boxed_fn) as *mut c_void;


        let opts = celix_service_use_options_t {
            filter: celix_service_filter_options_t {
                serviceName: c_service_name_ptr,
                versionRange: null_mut(),
                filter: c_filter_ptr,
                serviceLanguage: null_mut(),
                ignoreServiceLanguage: false,
            },
            waitTimeoutInSeconds: 0.0,
            callbackHandle: fn_ptr,
            use_: Some(Self::use_service_c_callback),
            useWithProperties: None,
            useWithOwner: None,
            flags: 0,
        };

        unsafe {
            if self.many {
                let count = celix_bundleContext_useServicesWithOptions(self.ctx.get_c_bundle_context(), &opts);
                Ok(count as isize)
            } else {
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

pub trait BundleContext {
    fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t;

    fn log(&self, level: LogLevel, message: &str);

    fn log_trace(&self, message: &str);

    fn log_debug(&self, message: &str);

    fn log_info(&self, message: &str);

    fn log_warning(&self, message: &str);

    fn log_error(&self, message: &str);

    fn log_fatal(&self, message: &str);

    fn register_service(&self) -> ServiceRegistrationBuilder;

    fn use_service(&self) -> ServiceUseBuilder;

    fn use_services(&self) -> ServiceUseBuilder;
}

struct BundleContextImpl {
    c_bundle_context: *mut celix_bundle_context_t,
    weak_self: Mutex<Option<Weak<BundleContextImpl>>>,
}

impl BundleContextImpl {
    fn new(c_bundle_context: *mut celix_bundle_context_t) -> Arc<Self> {
        let ctx = Arc::new(BundleContextImpl {
            c_bundle_context,
            weak_self: Mutex::new(None),
        });
        let weak_ref = Arc::downgrade(&ctx);
        ctx.set_self(weak_ref);
        ctx
    }

    fn set_self(&self, weak_self: Weak<BundleContextImpl>) {
        let mut guard = self.weak_self.lock().unwrap();
        *guard = Some(weak_self);
    }

    fn get_self(&self) -> Weak<BundleContextImpl> {
        self.weak_self.lock().unwrap().clone().unwrap()
    }

    fn unregister_service(&self, service_id: i64) {
        unsafe {
            celix_bundleContext_unregisterService(self.c_bundle_context, service_id);
        }
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
}

impl BundleContext for BundleContextImpl {
    fn get_c_bundle_context(&self) -> *mut celix_bundle_context_t {
        self.c_bundle_context
    }

    fn log(&self, level: LogLevel, message: &str) {
        self.log_to_c(level, message);
    }

    fn log_trace(&self, message: &str) {
        self.log(LogLevel::Trace, message);
    }

    fn log_debug(&self, message: &str) {
        self.log(LogLevel::Debug, message);
    }

    fn log_info(&self, message: &str) {
        self.log(LogLevel::Info, message);
    }

    fn log_warning(&self, message: &str) {
        self.log(LogLevel::Warning, message);
    }

    fn log_error(&self, message: &str) {
        self.log(LogLevel::Error, message);
    }

    fn log_fatal(&self, message: &str) {
        self.log(LogLevel::Fatal, message);
    }

    fn register_service(&self) -> ServiceRegistrationBuilder {
        ServiceRegistrationBuilder::new(self)
    }

    fn use_service(&self) -> ServiceUseBuilder {
        ServiceUseBuilder::new(self, false)
    }

    fn use_services(&self) -> ServiceUseBuilder {
        ServiceUseBuilder::new(self, true)
    }
}

pub fn bundle_context_new(c_bundle_context: *mut celix_bundle_context_t) -> Arc<dyn BundleContext> {
    BundleContextImpl::new(c_bundle_context)
}
