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

use super::BundleContext;
use super::Error;

pub trait BundleActivator {
    fn new(ctx: Arc<BundleContext>) -> Self;
    fn start(&mut self) -> Result<(), Error> {
        /* Default implementation */
        Ok(())
    }
    fn stop(&mut self) -> Result<(), Error> {
        /* Default implementation */
        Ok(())
    }
}

#[macro_export]
macro_rules! generate_bundle_activator {
    ($activator:ty) => {
        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_create(
            ctx: *mut $crate::details::CBundleContext,
            out: *mut *mut ::std::ffi::c_void,
        ) -> $crate::details::CStatus {
            let boxed_context = $crate::bundle_context_new(ctx);
            let mut arc_context = Arc::from(boxed_context);
            let activator = <$activator>::new(arc_context);
            *out = Box::into_raw(Box::new(activator)) as *mut ::std::ffi::c_void;
            $crate::CELIX_SUCCESS
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_start(
            handle: *mut ::std::ffi::c_void,
            ctx: *mut $crate::details::CBundleContext,
        ) -> $crate::details::CStatus {
            let activator = &mut *(handle as *mut $activator);
            let result = activator.start();
            match result {
                Ok(_) => $crate::CELIX_SUCCESS,
                Err(e) => e.into(),
            }
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_stop(
            handle: *mut ::std::ffi::c_void,
            ctx: *mut $crate::details::CBundleContext,
        ) -> $crate::details::CStatus {
            let activator = &mut *(handle as *mut $activator);
            let result = activator.stop();
            match result {
                Ok(_) => $crate::CELIX_SUCCESS,
                Err(e) => e.into(),
            }
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_destroy(
            handle: *mut ::std::ffi::c_void,
            _ctx: *mut $crate::details::CBundleContext,
        ) -> $crate::details::CStatus {
            let reclaimed_activator = Box::from_raw(handle as *mut $activator);
            drop(reclaimed_activator);
            $crate::CELIX_SUCCESS
        }
    };
}
