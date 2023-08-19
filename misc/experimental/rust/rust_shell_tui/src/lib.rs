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

use std::os::raw::c_char;
use std::os::raw::c_void;
use std::ffi::CString;
use std::ffi::NulError;
use std::ptr::null_mut;
use celix_bindings::*; //Add all Apache Celix C bindings to the namespace (i.e. celix_bundleContext_log, etc.)

// pub struct celix_shell_command {
//     pub handle
//     : *mut ::std ::os ::raw ::c_void,
//     #[doc =
//     " Calls the shell command.\n @param handle        The shell command handle.\n @param commandLine   The "
//     "complete provided cmd line (e.g. for a 'stop' command -> 'stop 42')\n @param outStream     The output "
//     "stream, to use for printing normal flow info.\n @param errorStream   The error stream, to use for "
//     "printing error flow info.\n @return              Whether a command is successfully executed."] pub
//     executeCommand
// pub executeCommand : :: std :: option :: Option < unsafe extern "C" fn (handle : * mut :: std :: os :: raw :: c_void , commandLine : * const :: std :: os :: raw :: c_char , outStream : * mut FILE , errorStream : * mut FILE) -> bool > , }
// }


struct RustShellTui {
    ctx: *mut celix_bundle_context_t,
    svc_id: i64,
    svc: celix_shell_command,
}

unsafe extern "C" fn rust_shell_tui_execute_command(_handle: *mut c_void, command_line: *const c_char, out_stream: *mut FILE, error_stream: *mut FILE) -> bool {
    let obj = Box::from_raw(_handle as *mut RustShellTui);
    obj.execute_command(command_line, out_stream, error_stream)
}

impl RustShellTui {

    unsafe fn new(ctx: *mut celix_bundle_context_t) -> Result<RustShellTui, NulError> {
        let result = RustShellTui {
            ctx,
            svc_id: -1,
            svc: celix_shell_command {
                handle: null_mut(),
                executeCommand: Some(rust_shell_tui_execute_command),
            },
        };
        Ok(result)
    }

    unsafe fn start(&mut self) -> Result<(), NulError> {

        // let mut input = String::new();
        // print!("-> ");
        // loop {
        //     std::io::stdin().read_line(&mut input).unwrap();
        //     println!("You typed: {}", input.trim());
        //     input.clear();
        //     print!("-> ");
        // }

        // self.svc.executeCommand = Some(|handle: *mut c_void, commandLine: *const c_char, outStream: *mut FILE, errorStream: *mut FILE| -> bool {
        //         println!("RustShellTui::executeCommand called");
        //         true
        // });

        //TODO let svc_name = CString::new(CELIX_SHELL_COMMAND_SERVICE_NAME as * const c_char).unwrap();
        let svc_name = CString::new("celix_shell_command").unwrap();

        let command_name = CString::new("command.name").unwrap();
        let props = celix_properties_create();
        celix_properties_set(props, command_name.as_ptr(), CString::new("rust").unwrap().as_ptr());

        self.svc.handle = Box::into_raw(Box::new(self.svc)) as *mut c_void;


        self.svc_id = celix_bundleContext_registerServiceAsync(
            self.ctx,
            Box::into_raw(Box::new(self.svc)) as *mut c_void,
            svc_name.as_ptr(),
            props);

        Ok(())
    }

    unsafe fn stop(&mut self) -> Result<(), NulError> {
        celix_bundleContext_unregisterService(self.ctx, self.svc_id);
        self.svc_id = -1;

        // let to_drop = Box::from_raw(self.svc.handle);
        // drop(to_drop);

        // TODO drop
        // let to_drop = Box::from_raw(&self.svc);
        // drop(to_drop);
        Ok(())
    }

    fn execute_command(&self, _command_line: *const c_char, _out_stream: *mut FILE, _error_stream: *mut FILE) -> bool {
        println!("RustShellTui::executeCommand called");
        true
    }
}

impl Drop for RustShellTui {
    fn drop(&mut self) { () }
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_create(ctx: *mut celix_bundle_context_t, data: *mut *mut c_void) -> celix_status_t {
    let obj = RustShellTui::new(ctx);
    if obj.is_err() {
        return CELIX_BUNDLE_EXCEPTION;
    }
    *data = Box::into_raw(Box::new(obj.unwrap())) as *mut c_void;
    CELIX_SUCCESS
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_start(data: *mut c_void, _ctx: *mut celix_bundle_context_t) -> celix_status_t {
    let obj = &mut *(data as *mut RustShellTui);
    let result = obj.start();
    match result {
        Ok(()) => CELIX_SUCCESS,
        Err(_) => CELIX_BUNDLE_EXCEPTION,
    }
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_stop(data: *mut c_void, _ctx: *mut celix_bundle_context_t) -> celix_status_t {
    let obj = &mut *(data as *mut RustShellTui);
    let result = obj.stop();
    match result {
        Ok(()) => CELIX_SUCCESS,
        Err(_) => CELIX_BUNDLE_EXCEPTION,
    }
}

#[no_mangle]
pub unsafe extern "C" fn celix_bundleActivator_destroy(data: *mut c_void, _ctx: *mut celix_bundle_context_t) -> celix_status_t {
    let obj = Box::from_raw(data as *mut RustShellTui);
    drop(obj);
    CELIX_SUCCESS
}

