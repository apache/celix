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

use celix_bindings::*; //Add all Apache Celix C bindings to the namespace (i.e. celix_bundleContext_log, etc.)
use celix::BundleActivator;
use celix::BundleContext;
use celix::Error;

struct HelloWorldBundle {}

impl BundleActivator for HelloWorldBundle {
    fn new(ctx: &mut dyn celix::BundleContext) -> Self {
        ctx.log_info("Hello World Bundle Activator created");
        HelloWorldBundle{}
    }

    fn start(&mut self, ctx: &mut dyn BundleContext) -> Result<(), Error> {
        ctx.log_info("Hello World Bundle Activator started");
        Ok(())
    }

    fn stop(&mut self, ctx: &mut dyn BundleContext) -> Result<(), Error> {
        ctx.log_info("Hello World Bundle Activator stopped");
        Ok(())
    }
}

impl Drop for HelloWorldBundle {
    fn drop(&mut self) {
        //TODO self.ctx.log_info("Hello World Bundle Activator dropped");
    }
}

generate_bundle_activator!(HelloWorldBundle);