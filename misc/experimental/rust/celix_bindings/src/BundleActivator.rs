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

pub trait BundleActivator {
    fn new(ctx: &mut dyn BundleContext) -> Self;
    fn start(&mut self, ctx: &mut dyn BundleContext) -> celix_status_t { /* Default implementation */ CELIX_SUCCESS}
    fn stop(&mut self, ctx: &mut dyn BundleContext) -> celix_status_t { /* Default implementation */ CELIX_SUCCESS}
}

#[macro_export]
macro_rules! generate_bundle_activator {
    ($activator:ident) => {
        use celix::BundleContextImpl;
        use CELIX_SUCCESS;
        use celix_bindings::celix_bundle_context_t;

        struct ActivatorWrapper {
            ctx: BundleContextImpl,
            activator: $activator,
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_create(
            ctx: *mut celix_bundle_context_t,
            out: *mut *mut c_void,
        ) -> celix_status_t {
            let mut context = BundleContextImpl::new(ctx);
            let activator = $activator::new(&mut context);
            let wrapper = ActivatorWrapper {
                ctx: context,
                activator
            };
            *out = Box::into_raw(Box::new(wrapper)) as *mut c_void;
            CELIX_SUCCESS
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_start(
            handle: *mut c_void,
            ctx: *mut celix_bundle_context_t,
        ) -> celix_status_t {
            let wrapper = &mut *(handle as *mut ActivatorWrapper);
            wrapper.activator.start(&mut wrapper.ctx)
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_stop(
            handle: *mut c_void,
            ctx: *mut celix_bundle_context_t,
        ) -> celix_status_t {
            let wrapper = &mut *(handle as *mut ActivatorWrapper);
            wrapper.activator.stop(&mut wrapper.ctx)
        }

        #[no_mangle]
        pub unsafe extern "C" fn celix_bundleActivator_destroy(handle: *mut c_void) -> celix_status_t {
            let wrapper = Box::from_raw(handle as *mut ActivatorWrapper);
            drop(wrapper);
            CELIX_SUCCESS
        }
    };
}
