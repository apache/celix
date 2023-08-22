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


struct CShellCommandImpl {
    ctx: Arc<dyn BundleContext>,
}

impl CShellCommandImpl {
    fn new(ctx: Arc<dyn BundleContext>) -> Self {
        ctx.log_info("Shell Command created");
        CShellCommandImpl {
            ctx,
        }
    }

    extern "C" fn call_execute_command(handle: *mut c_void, command_line: *const c_char, _out_stream: *mut FILE, _error_stream: *mut FILE) -> bool {
        if handle.is_null() || command_line.is_null() {
            return false;
        }
        unsafe {
            println!("call_execute_command");
            let obj = &mut *(handle as *mut CShellCommandImpl);
            let str_command_line = std::ffi::CStr::from_ptr(command_line).to_str().unwrap();
            obj.execute_command(str_command_line);
        }
        true
    }

    fn execute_command(&mut self, command_line: &str) {
        self.ctx.log_info(format!("Execute command: {}", command_line).as_str());
    }
}

//temporary, should be moved in a separate API crate
trait RustShellCommand {
    fn execute_command(&mut self, command_line: &str) -> Result<(), Error>;
}

struct RustShellCommandImpl {
    ctx: Arc<dyn BundleContext>,
}

impl RustShellCommandImpl {
    fn new(ctx: Arc<dyn BundleContext>) -> Self {
        ctx.log_info("Rust Shell Command created");
        RustShellCommandImpl {
            ctx,
        }
    }
}

impl RustShellCommand for RustShellCommandImpl {
    fn execute_command(&mut self, command_line: &str) -> Result<(), Error> {
        self.ctx.log_info(format!("Execute command: {}.", command_line).as_str());
        Ok(())
    }
}

struct ShellCommandActivator {
    ctx: Arc<dyn BundleContext>,
    //log_helper: Box<dyn LogHelper>,
    shell_command_provider: CShellCommandImpl,
    c_registration: Option<celix::ServiceRegistration>,
    rust_registration: Option<celix::ServiceRegistration>,
}

impl BundleActivator for ShellCommandActivator {
    fn new(ctx: Arc<dyn celix::BundleContext>) -> Self {
        let result = ShellCommandActivator {
            ctx: ctx.clone(),
            shell_command_provider: CShellCommandImpl::new(ctx.clone()),
            //log_helper: log_helper_new(&*ctx, "ShellCommandBundle"),
            c_registration: None,
            rust_registration: None,
        };
        result
    }
    fn start(&mut self) -> Result<(), Error> {
        let registration = self.ctx.register_service()
            .with_service( celix_shell_command_t{
                handle: &mut self.shell_command_provider as *mut CShellCommandImpl as *mut c_void,
                executeCommand: Some(CShellCommandImpl::call_execute_command),
                })
            .with_service_name("celix_shell_command")
            .with_property("command.name", "exe_in_rust")
            .with_property("command.description", "Simple command written in Rust")
            .build()?;
        self.c_registration = Some(registration);
        self.ctx.log_info("C Shell Command registered");

        let rust_shell_command = Box::new(RustShellCommandImpl::new(self.ctx.clone()));
        let registration = self.ctx.register_service()
            //maybe make svc types more explicit, e.g.with type parameters
            .with_service(rust_shell_command as Box<dyn RustShellCommand>)
            .with_property("command.name", "exe_in_rust")
            .with_property("command.description", "Simple command written in Rust")
            .build()?;
        self.rust_registration = Some(registration);

        self.ctx.log_info("Rust Shell Command started");
        Ok(())
    }

    fn stop(&mut self) -> Result<(), Error> {
        self.rust_registration = None;
        self.c_registration = None;
        self.ctx.log_info("Rust Shell Command stopped");
        Ok(())
    }
}

celix::generate_bundle_activator!(ShellCommandActivator);
