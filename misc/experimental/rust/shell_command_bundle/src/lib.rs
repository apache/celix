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

use std::ffi::c_char;
use std::ffi::c_void;
use std::sync::Arc;

use celix::{BundleActivator, LogHelper};
use celix::BundleContext;
use celix::Error;
use rust_shell_api::RustShellCommand;

use celix_bindings::celix_shell_command_t;
use celix_bindings::FILE;

struct CShellCommandImpl {
    ctx: Arc<BundleContext>,
}

impl CShellCommandImpl {
    fn new(ctx: Arc<BundleContext>) -> Self {
        ctx.log_info("Shell Command created");
        CShellCommandImpl { ctx }
    }

    extern "C" fn call_execute_command(
        handle: *mut c_void,
        command_line: *const c_char,
        _out_stream: *mut FILE,
        _error_stream: *mut FILE,
    ) -> bool {
        if handle.is_null() || command_line.is_null() {
            return false;
        }
        unsafe {
            let obj = &mut *(handle as *mut CShellCommandImpl);
            let str_command_line = std::ffi::CStr::from_ptr(command_line).to_str();
            if str_command_line.is_err() {
                return false;
            }
            obj.execute_command(str_command_line.unwrap());
        }
        true
    }

    fn execute_command(&mut self, command_line: &str) {
        self.ctx.log_info(format!("Execute command: \"{}\"", command_line).as_str());
    }
}

//temporary, should be moved in a separate API crate

struct RustShellCommandImpl {
    ctx: Arc<BundleContext>,
}

impl RustShellCommandImpl {
    fn new(ctx: Arc<BundleContext>) -> Self {
        ctx.log_info("Rust Shell Command created");
        RustShellCommandImpl { ctx }
    }
}

impl RustShellCommand for RustShellCommandImpl {
    fn execute_command(&self, command_line: &str) -> Result<(), Error> {
        self.ctx
            .log_info(format!("Execute command: {}.", command_line).as_str());
        Ok(())
    }
}

struct ShellCommandActivator {
    ctx: Arc<BundleContext>,
    log_helper: Arc<LogHelper>,
    shell_command_provider: CShellCommandImpl,
    registrations: Vec<celix::ServiceRegistration>,
}

impl ShellCommandActivator {

    fn register_services(&mut self) -> Result<(), Error> {
        //Register C service registered as value
        let registration = self.ctx.register_service()
            .with_service(celix_shell_command_t {
                handle: &mut self.shell_command_provider as *mut CShellCommandImpl as *mut c_void,
                executeCommand: Some(CShellCommandImpl::call_execute_command),
            })
            .with_service_name("celix_shell_command")
            .with_property("command.name", "exe_c_command_in_rust")
            .with_property("command.description", "Simple command written in Rust")
            .build()?;
        self.registrations.push(registration);
        self.log_helper.log_info("C Shell Command registered");

        //Register Rust trait service register using a Arc
        let rust_shell_command: Arc<dyn RustShellCommand> = Arc::new(RustShellCommandImpl::new(self.ctx.clone()));
        let registration = self.ctx.register_service()
            .with_service(rust_shell_command)
            .with_property(rust_shell_api::COMMAND_NAME, "exe_rust_command")
            .with_property(rust_shell_api::COMMAND_DESCRIPTION, "Simple command written in a Rust trait")
            .build()?;
        self.registrations.push(registration);
        self.log_helper.log_info("Rust trait Shell Command registered");


        Ok(())
    }

    fn use_services(&mut self) -> Result<(), Error> {
        //test using C service

        self.log_helper.log_info("Use C service command service");
        let count = self.ctx.use_services()
            .with_service_name("celix_shell_command")
            .with_filter("(command.name=exe_c_command_in_rust)")
            .with_callback(Box::new( |svc: &celix_shell_command_t| {
                if let Some(exe_cmd) = svc.executeCommand {
                    let c_str = std::ffi::CString::new("test c service").unwrap();
                    unsafe {
                        exe_cmd(svc.handle, c_str.as_ptr() as *const c_char, std::ptr::null_mut(), std::ptr::null_mut());
                    }
                }
            }))
            .build()?;
        self.log_helper.log_info(format!("Found {} celix_shell_command_t services", count).as_str());

        //test using Rust trait service
        self.log_helper.log_info("Use Rust trait service command service");
        let count = self.ctx.use_services()
            .with_callback(Box::new( |svc: &Arc<dyn RustShellCommand>| {
                let _ = svc.execute_command("test rest trait");
            }))
            .build()?;
        self.log_helper.log_info(format!("Found {} RustShellCommandTrait services", count).as_str());

        Ok(())
    }
}

impl BundleActivator for ShellCommandActivator {
    fn new(ctx: Arc<BundleContext>) -> Self {
        let result = ShellCommandActivator {
            ctx: ctx.clone(),
            shell_command_provider: CShellCommandImpl::new(ctx.clone()),
            log_helper: LogHelper::new(ctx.clone(), "ShellCommandBundle"),
            registrations: Vec::new(),
        };
        result
    }
    fn start(&mut self) -> Result<(), Error> {
        self.register_services()?;
        self.use_services()?;
        self.ctx.log_info("Rust Shell Command started");
        Ok(())
    }

    fn stop(&mut self) -> Result<(), Error> {
        self.registrations.clear();
        self.ctx.log_info("Rust Shell Command stopped");
        Ok(())
    }
}

celix::generate_bundle_activator!(ShellCommandActivator);
