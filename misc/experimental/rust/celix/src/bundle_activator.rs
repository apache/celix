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
use ::celix::Error;

pub trait BundleActivator {
    fn new(ctx: &mut dyn BundleContext) -> Self;
    fn start(&mut self, _ctx: &mut dyn BundleContext) -> Result<(), Error> { /* Default implementation */ Ok(())}
    fn stop(&mut self, _ctx: &mut dyn BundleContext)  -> Result<(), Error> { /* Default implementation */ Ok(())}
}

#[macro_export]
macro_rules! generate_bundle_activator {
    ($activator:ty) => {
        // fn assert_implements_trait(_: &$activator) {
        //     fn must_implement<T: celix::BundleActivator>(_t: &T) {}
        //     must_implement(&*(None::<$activator>).unwrap_or_default());
        // }

        struct ActivatorWrapper {
            ctx: Box<dyn $crate::celix::BundleContext>,
            activator: $activator,
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_create(
            ctx: *mut $crate::celix_bundle_context_t,
            out: *mut *mut ::std::ffi::c_void,
        ) -> $crate::celix_status_t {
            let mut context = $crate::celix::create_bundle_context_instance(ctx);
            let activator = <$activator>::new(&mut *context);
            let wrapper = ActivatorWrapper {
                ctx: context,
                activator
            };
            *out = Box::into_raw(Box::new(wrapper)) as *mut ::std::ffi::c_void;
            $crate::celix::CELIX_SUCCESS
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_start(
            handle: *mut ::std::ffi::c_void,
            ctx: *mut $crate::celix_bundle_context_t,
        ) -> $crate::celix_status_t {
            let wrapper = &mut *(handle as *mut ActivatorWrapper);
            let result = wrapper.activator.start(&mut *wrapper.ctx);
            match result {
                Ok(_) => $crate::celix::CELIX_SUCCESS,
                Err(e) => e.into(),
            }
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_stop(
            handle: *mut ::std::ffi::c_void,
            ctx: *mut $crate::celix_bundle_context_t,
        ) -> $crate::celix_status_t {
            let wrapper = &mut *(handle as *mut ActivatorWrapper);
            let result = wrapper.activator.stop(&mut *wrapper.ctx);
            match result {
                Ok(_) => $crate::celix::CELIX_SUCCESS,
                Err(e) => e.into(),
            }
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_destroy(
            handle: *mut ::std::ffi::c_void
        ) -> $crate::celix_status_t {
            let reclaimed_wrapper = Box::from_raw(handle as *mut ActivatorWrapper);
            drop(reclaimed_wrapper);
            $crate::celix::CELIX_SUCCESS
        }
    };
}
