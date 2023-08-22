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
extern crate celix;

use std::sync::Arc;
use std::ffi::c_void;
use std::ffi::c_char;

use celix::BundleActivator;
use celix::BundleContext;
use celix::Error;

use celix_bindings::celix_shell_command_t;
use celix_bindings::FILE;

//temporary, should be moved in a separate API crate
// trait RustShellCommand {
//     fn execute_command(&mut self, command_line: &str, command_args: Vec<&str>) -> Result<(), Error>;
// }

struct ShellCommandProvider {
    ctx: Arc<dyn BundleContext>,
}
impl ShellCommandProvider {
    fn new(ctx: Arc<dyn BundleContext>) -> Self {
        ctx.log_info("Shell Command created");
        ShellCommandProvider{
            ctx,
        }
    }

    extern "C" fn call_execute_command(handle: *mut c_void, command_line: *const c_char, _out_stream: *mut FILE, _error_stream: *mut FILE) -> bool {
        if handle.is_null() || command_line.is_null() {
            return false;
        }
        unsafe {
            println!("call_execute_command");
            let obj = &mut *(handle as *mut ShellCommandProvider);
            let str_command_line = std::ffi::CStr::from_ptr(command_line).to_str().unwrap();
            obj.execute_command(str_command_line);
        }
        true
    }

    fn execute_command(&mut self, command_line: &str) {
        self.ctx.log_info(format!("Execute command: {}", command_line).as_str());
    }
}

struct ShellCommandActivator {
    ctx: Arc<dyn BundleContext>,
    //log_helper: Box<dyn LogHelper>,
    shell_command_provider: ShellCommandProvider,
    registration: Option<celix::ServiceRegistration>,
}

impl BundleActivator for ShellCommandActivator {
    fn new(ctx: Arc<dyn celix::BundleContext>) -> Self {
        let result = ShellCommandActivator {
            ctx: ctx.clone(),
            shell_command_provider: ShellCommandProvider::new(ctx.clone()),
            //log_helper: log_helper_new(&*ctx, "ShellCommandBundle"),
            registration: None,
        };
        result
    }
    fn start(&mut self) -> Result<(), Error> {
        let registration = self.ctx.register_service()
            .with_service( celix_shell_command_t{
                handle: &mut self.shell_command_provider as *mut ShellCommandProvider as *mut c_void,
                executeCommand: Some(ShellCommandProvider::call_execute_command),
                })
            .with_service_name("celix_shell_command")
            .with_property("command.name", "exe_in_rust")
            .with_property("command.description", "Simple command written in Rust")
            .build()?;
        self.registration = Some(registration);
        self.ctx.log_info("Rust Shell Command started");
        Ok(())
    }

    fn stop(&mut self) -> Result<(), Error> {
        self.registration = None;
        self.ctx.log_info("Rust Shell Command stopped");
        Ok(())
    }
}

celix::generate_bundle_activator!(ShellCommandActivator);
